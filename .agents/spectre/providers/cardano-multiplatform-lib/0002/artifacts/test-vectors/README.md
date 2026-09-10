# Cardano test vectors for capture 0002

The selected vectors are unchanged from [0001](../../../0001/SNAPSHOT.md).
The local [manifest](manifest.json) rebases fixturePath to the existing 0001 files under .agents/.
No vector bytes, era metadata, storage form or expected outcome changes; the historical manifest
stays immutable at its original path.

The new [PROVENANCE.json](PROVENANCE.json) records this capture's CML source revision and retains
Dolos/Pallas origins. Its fixtureInventory and licenseFile paths resolve relative to this directory.
The supplementalSha256 key PROVENANCE.md denotes the logical test-vectors/PROVENANCE.md artifact;
resolve it through [SNAPSHOT.md](../../SNAPSHOT.md), rather than assuming a local copy exists.

Reuse [upstream provenance](../../../0001/artifacts/test-vectors/PROVENANCE.md),
[Apache-2.0 text](../../../0001/artifacts/test-vectors/LICENSE-APACHE-2.0.txt), and
[CML license](../../../0001/artifacts/legal/LICENSE). Captured files are inert evidence.
The complete logical inventory, exact counts and integrity rules belong to
[SNAPSHOT.md](../../SNAPSHOT.md); findings belong to [CAPTURE.md](../../CAPTURE.md).
