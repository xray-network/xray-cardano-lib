import { defineConfig } from "@rspress/core"
import { documentationSections } from "./navigation"

export default defineConfig({
  root: "src",
  outDir: "build/xray-cardano-lib",
  base: "/xray-cardano-lib/",
  siteOrigin: "https://wiki.xraynetwork.io",
  icon: "https://cdn.xraynetwork.io/favicon.png",
  themeDir: "theme",
  title: "XRAY Cardano Lib",
  logo: "/xray-blue.svg",
  logoText: "CARDANO LIB",
  description: "XRAY Cardano Lib documentation.",
  head: [
    ["meta", { property: "og:type", content: "website" }],
    ["meta", { property: "og:title", content: "XRAY Cardano Lib — Protocol-grade Cardano primitives" }],
    ["meta", { property: "og:description", content: "Modular, lossless Cardano libraries for TypeScript." }]
  ],
  route: { cleanUrls: true },
  themeConfig: {
    fallbackHeadingTitle: false,
    darkMode: "dark",
    enableAppearanceAnimation: false,
    nav: [
      {
        text: "Back to Wiki",
        link: "https://wiki.xraynetwork.io",
        icon: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="m12 5-7 7 7 7M5 12h14"/></svg>',
        position: "left"
      }
    ],
    sidebar: Object.fromEntries(
      documentationSections.flatMap(({ paths, sidebar }) =>
        paths.map(path => [path, sidebar])
      )
    )
  }
})
