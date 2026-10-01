# README visual system

Five English, self-contained SVG diagrams. Diagram 1 retains the original
architecture look. Diagrams 2–5 are full infographic scenes inspired by the
supplied references: dark perspective stages, exploded transparent planes,
glowing signal ribbons, isometric data objects and larger plotted structures in
cyan / violet / mint / amber / pink. Text remains real vector lettering; no
external images, fonts, scripts or services are required.

| Asset | Purpose |
|---|---|
| `01-system.svg` | Layered architecture, dependencies, control and telemetry |
| `02-frame-pipeline.svg` | Exploded packet → route mask → complete frame stack |
| `03-run-and-test.svg` | Octo adapter scene, eight output lanes and test states |
| `04-performance.svg` | Proportional 3D columns for measured counts and bottlenecks |
| `05-diagnostics.svg` | Four-layer diagnostic stack and tuning path |

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
