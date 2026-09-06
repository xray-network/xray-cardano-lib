import { useId } from "react"

export function HeroDiagram() {
  const id = useId()
  return (
    <svg className="spectre-hero-diagram" viewBox="0 60 320 134" role="img" aria-labelledby={`${id}-title ${id}-description`} focusable="false">
      <title id={`${id}-title`}>XRAY Cardano Lib architecture.</title>
      <desc id={`${id}-description`}>Encode Cardano data, sign securely, then build protocol operations.</desc>
      <rect className="spectre-diagram-frame" x="14" y="68" width="292" height="118" rx="8" />

      <g className="spectre-diagram-records">
        <rect x="36" y="84" width="56" height="48" rx="4" />
        <rect x="132" y="84" width="56" height="48" rx="4" />
        <rect x="228" y="84" width="56" height="48" rx="4" />
        <g className="spectre-diagram-icon">
          <path d="M56 96h11l5 5v19H56Zm11 0v6h5m-12 5h8m-8 6h8" />
          <path d="m154 101-7 7 7 7m12-14 7 7-7 7m-4-19-4 24" />
          <path d="m246 108 7 7 13-15" />
        </g>
        <text x="64" y="151">Encode</text>
        <text x="160" y="151">Sign</text>
        <text x="256" y="151">Build</text>
      </g>
      <g className="spectre-diagram-arrows">
        <path className="spectre-diagram-dashed" d="M96 108h28M192 108h28" />
        <path d="M120 104L124 108L120 112M216 104L220 108L216 112" />
      </g>
      <g className="spectre-diagram-caption">
        <text x="64" y="169">Lossless data</text>
        <text x="160" y="169">Safe crypto</text>
        <text x="256" y="169">Typed flows</text>
      </g>
    </svg>
  )
}
