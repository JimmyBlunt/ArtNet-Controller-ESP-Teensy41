# README visual system

Five English, self-contained SVG diagrams, revised 2 October 2026. Dark navy,
transparent layers, fine circuit traces and restrained cyan / violet / mint /
amber / pink glow follow the supplied visual references. All text remains real
vector text; no external images, fonts, scripts or services are required.

| Asset | Purpose |
|---|---|
| `01-system.svg` | Layered architecture, dependencies, control and telemetry |
| `02-frame-pipeline.svg` | ArtDmx bytes, validation, route mask, short payloads |
| `03-run-and-test.svg` | Automatic start, recovery and port-test workflow |
| `04-performance.svg` | Actual recorded frame counts and timing constraints |
| `05-diagnostics.svg` | Counter progression, errors and tuning opportunities |

Regenerate from the repository root:

```powershell
python tools/render_readme_diagrams.py
```

The performance diagram reads the archived CSV directly. Technical claims,
source links and limitations are documented in [the English guide](../../ARTNET_DATA_FLOW.md).
Each SVG includes a title and descriptive alternative text. README prose and
the guide provide a readable text equivalent when small-screen diagrams require zoom.

Browser QA uses `output/playwright/readme-diagrams.html` and the Playwright CLI
callback `tools/check_readme_diagrams.js`. Generated screenshots are local QA
artifacts, not firmware assets. The supplied reference images are not copied
into the repository; only their general style informs these original diagrams.
