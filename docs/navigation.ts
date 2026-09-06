import type { Sidebar } from "@rspress/core"

const implementation = (target: string, id: string, title: string) => ({
  text: `${id} — ${title}`,
  collapsed: true,
  items: [
    { text: "Instruction", link: `/impl/${target}/${id}-IMPL-INSTR` },
    { text: "Result", link: `/impl/${target}/${id}-IMPL-RESULT` }
  ]
})

export const documentationSections = [
  {
    text: "Docs",
    href: "/",
    paths: ["/", "/adr", "/impl"],
    icon: "M3 10 12 3l9 7M5 9v12h5v-7h4v7h5V9",
    sidebar: [
      { sectionHeaderText: "Overview" },
      { text: "XRAY Cardano Lib", link: "/" },
      { sectionHeaderText: "Architecture decisions" },
      {
        text: "Repository",
        collapsed: true,
        items: [
          { text: "XRAY Updates v1 installation", link: "/adr/repository/0001-xray-updates-standard" }
        ]
      },
      {
        text: "TypeScript",
        collapsed: true,
        items: [
          { text: "Lossless CBOR and encoding metadata", link: "/adr/typescript/0001-lossless-cbor-and-encoding-metadata" },
          { text: "Cryptography dependency policy", link: "/adr/typescript/0002-cryptography-dependency-policy" },
          { text: "Upstream evidence and package ownership", link: "/adr/typescript/0003-upstream-evidence-and-package-ownership" },
          { text: "Cryptography primitives", link: "/adr/typescript/0004-cryptography-primitives" }
        ]
      },
      { sectionHeaderText: "Implementation history" },
      { text: "Aggregate status", link: "/impl/XRAY-UPDATES-STATUS" },
      {
        text: "Repository",
        collapsed: true,
        items: [implementation("repository", "0001", "Install XRAY Updates")]
      },
      {
        text: "TypeScript",
        collapsed: true,
        items: [
          implementation("typescript", "0001", "CML baseline"),
          implementation("typescript", "0002", "Message signing"),
          implementation("typescript", "0003", "UPLC implementation"),
          implementation("typescript", "0004", "Cardano Ledger validation"),
          implementation("typescript", "0005", "Value and transaction-builder correctness"),
          implementation("typescript", "0006", "Typed Conway governance construction"),
          implementation("typescript", "0007", "CIP-14 asset fingerprints"),
          implementation("typescript", "0008", "Strict identities and HD derivation"),
          implementation("typescript", "0009", "CIP-57 Plutus contract blueprints"),
          implementation("typescript", "0010", "CIP-67/68 token metadata"),
          implementation("typescript", "0011", "Optional CIP-21 compatibility diagnostics"),
          implementation("typescript", "0012", "XRAY Cardano Lib package-family rename"),
          implementation("typescript", "0013", "Typed Plutus void and aggregate exports"),
          implementation("typescript", "0014", "CIP8Message facade"),
          implementation("typescript", "0015", "CostModels JSON parsing and validation"),
          implementation("typescript", "0016", "CIP-4 wallet checksum facade"),
          implementation("typescript", "0017", "Typed transaction decomposition"),
          implementation("typescript", "0018", "Required-witness discovery"),
          implementation("typescript", "0019", "Typed scripts and serialized envelopes"),
          implementation("typescript", "0020", "Typed phase-two redeemer identity"),
          implementation("typescript", "0021", "CostModels public consumption identity"),
          implementation("typescript", "0022", "Normalize protocol export namespaces"),
          implementation("typescript", "0023", "Canonicalize constructed ADA-only values"),
          implementation("typescript", "0024", "Complete transaction inspection accessors")
        ]
      },
      {
        text: "C++ concept",
        collapsed: true,
        items: [
          implementation("cpp", "0001", "Full TypeScript feature parity"),
          implementation("cpp", "0002", "Builder and typed Conway construction hardening"),
          implementation("cpp", "0003", "Portability, lean components, and benchmarks"),
          implementation("cpp", "0004", "CIP-14 asset fingerprints"),
          implementation("cpp", "0005", "Strict identities and HD derivation"),
          implementation("cpp", "0006", "CIP-57 Plutus contract blueprints"),
          implementation("cpp", "0007", "CIP-67/68 token metadata"),
          implementation("cpp", "0008", "Optional CIP-21 compatibility diagnostics"),
          implementation("cpp", "0009", "Local CMake test ownership"),
          implementation("cpp", "0010", "XRAY Cardano Lib package rename")
        ]
      }
    ]
  }
] satisfies { text: string; href: string; paths: string[]; icon: string; sidebar: Sidebar[string] }[]
