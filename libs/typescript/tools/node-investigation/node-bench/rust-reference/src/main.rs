use std::{hint::black_box, io::{self, BufRead, Write}, time::Instant};
use blake2::{Blake2bVar, digest::{Update, VariableOutput}};
use ed25519_dalek::{SigningKey, VerifyingKey, Signature, Signer};
use k256::ecdsa::signature::hazmat::PrehashVerifier;
use serde_json::{Value, json};
use sha2::Digest;

fn bytes(v: &Value, field: &str) -> Vec<u8> { hex::decode(v[field].as_str().unwrap()).unwrap() }
struct Ed { key: VerifyingKey, seed: SigningKey, sig: Signature, msg: Vec<u8> }
struct Secp { key: k256::ecdsa::VerifyingKey, sig: k256::ecdsa::Signature,
    schnorr_key: k256::schnorr::VerifyingKey, schnorr_sig: k256::schnorr::Signature, msg: Vec<u8>,
    raw_key: Vec<u8>, raw_sig: Vec<u8>, raw_schnorr_key: Vec<u8>, raw_schnorr_sig: Vec<u8> }
struct Fixtures { ed: Vec<Ed>, secp: Vec<Secp>, hash: Vec<Vec<u8>> }
impl Fixtures {
    fn new(v: &Value) -> Self {
        Self {
            ed: v["ed"].as_array().unwrap().iter().map(|x| Ed {
                key: VerifyingKey::from_bytes(&bytes(x,"public").try_into().unwrap()).unwrap(),
                seed: SigningKey::from_bytes(&bytes(x,"seed").try_into().unwrap()),
                sig: Signature::from_slice(&bytes(x,"signature")).unwrap(), msg: bytes(x,"message")
            }).collect(),
            secp: v["secp"].as_array().unwrap().iter().map(|x| Secp {
                key: k256::ecdsa::VerifyingKey::from_sec1_bytes(&bytes(x,"public")).unwrap(),
                sig: k256::ecdsa::Signature::from_slice(&bytes(x,"signature")).unwrap(),
                schnorr_key: k256::schnorr::VerifyingKey::from_bytes(&bytes(x,"schnorrPublic")).unwrap(),
                schnorr_sig: k256::schnorr::Signature::try_from(bytes(x,"schnorrSignature").as_slice()).unwrap(),
                msg: bytes(x,"message"), raw_key: bytes(x,"public"), raw_sig: bytes(x,"signature"),
                raw_schnorr_key: bytes(x,"schnorrPublic"), raw_schnorr_sig: bytes(x,"schnorrSignature")
            }).collect(),
            hash: v["hash"].as_array().unwrap().iter().map(|x|hex::decode(x.as_str().unwrap()).unwrap()).collect(),
        }
    }
    fn op(&self, op: &str, i: usize) -> Vec<u8> {
        let e = &self.ed[i % self.ed.len()];
        let c = &self.secp[i % self.secp.len()];
        match op {
            "ed25519_verify" => vec![e.key.verify_strict(black_box(&e.msg), &e.sig).is_ok() as u8],
            "ed25519_verify_fresh" => {
                let key = VerifyingKey::from_bytes(black_box(e.key.as_bytes())).unwrap();
                let sig = Signature::from_bytes(black_box(&e.sig.to_bytes()));
                vec![key.verify_strict(black_box(&e.msg), &sig).is_ok() as u8]
            },
            "ed25519_sign" => e.seed.sign(black_box(&e.msg)).to_bytes().to_vec(),
            // Include parsing every check: XRAY's public secp primitive also accepts raw bytes.
            "secp256k1_ecdsa_verify" => {
                let key = k256::ecdsa::VerifyingKey::from_sec1_bytes(black_box(&c.raw_key)).unwrap();
                let sig = k256::ecdsa::Signature::from_slice(black_box(&c.raw_sig)).unwrap();
                vec![key.verify_prehash(black_box(&c.msg), &sig).is_ok() as u8]
            },
            "secp256k1_schnorr_verify" => {
                let key = k256::schnorr::VerifyingKey::from_bytes(black_box(&c.raw_schnorr_key)).unwrap();
                let sig = k256::schnorr::Signature::try_from(black_box(c.raw_schnorr_sig.as_slice())).unwrap();
                vec![key.verify_raw(black_box(&c.msg), &sig).is_ok() as u8]
            },
            _ => {
                let (name, size) = op.rsplit_once('_').unwrap();
                let size: usize = size.parse().unwrap();
                let data = black_box(&self.hash.iter().find(|x| x.len() == size).unwrap()[..]);
                match name {
                    "blake2b256" | "blake2b224" => {
                        let mut output = vec![0; if name == "blake2b256" {32} else {28}];
                        let mut h = Blake2bVar::new(output.len()).unwrap();
                        Update::update(&mut h, data);
                        h.finalize_variable(&mut output).unwrap(); output
                    },
                    "sha2_256" => sha2::Sha256::digest(data).to_vec(),
                    "sha3_256" => sha3::Sha3_256::digest(data).to_vec(),
                    _ => panic!("unknown operation {op}")
                }
            }
        }
    }
}
fn main() {
    let stdin=io::stdin(); let mut lines=stdin.lock().lines();
    let fixtures=Fixtures::new(&serde_json::from_str::<Value>(&lines.next().unwrap().unwrap()).unwrap());
    for e in &fixtures.ed {
        assert!(e.key.verify_strict(&e.msg,&e.sig).is_ok());
        let mut bad=e.msg.clone();bad[0]^=1;
        assert!(e.key.verify_strict(&bad,&e.sig).is_err());
        assert_eq!(e.seed.sign(&e.msg).to_bytes(),e.sig.to_bytes());
    }
    for c in &fixtures.secp {
        assert!(c.key.verify_prehash(&c.msg,&c.sig).is_ok());
        assert!(c.schnorr_key.verify_raw(&c.msg,&c.schnorr_sig).is_ok());
        let mut bad=c.msg.clone();bad[0]^=1;
        assert!(c.key.verify_prehash(&bad,&c.sig).is_err());
        assert!(c.schnorr_key.verify_raw(&bad,&c.schnorr_sig).is_err());
    }
    println!("{}",json!({"ready":true}));io::stdout().flush().unwrap();
    for line in lines {
        let request: Value=serde_json::from_str(&line.unwrap()).unwrap();
        let op=request["op"].as_str().unwrap();
        let ms=request["ms"].as_f64().unwrap();
        let batch=request["batch"].as_u64().unwrap() as usize;
        let start=Instant::now();let mut count=0usize;
        loop {
            for _ in 0..batch {black_box(fixtures.op(op,black_box(count)));count+=1;}
            if start.elapsed().as_secs_f64()*1000.0>=ms {break;}
        }
        let elapsed_ms=start.elapsed().as_secs_f64()*1000.0;
        println!("{}",json!({"count":count,"elapsedMs":elapsed_ms,
            "opsPerSecond":count as f64 * 1000.0/elapsed_ms,"output":hex::encode(fixtures.op(op,0))}));
        io::stdout().flush().unwrap();
    }
}
