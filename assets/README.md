# Assets

All non-code assets for stagePLTR, organized by purpose.

```
assets/
├── plot/        Stage-plot symbol library — the in-app draggable items
│                (instruments, mics, monitors, DI boxes, risers, …). SVG (vector).
├── ui/          Application-chrome glyphs — toolbar/cursor icons, etc.
│                (distinct from the app icon and from plot symbols). SVG.
└── app-icon/    Platform application icon: iconsmaker source + config + generated
                 output. See assets/app-icon/README.md.
```

## Conventions

- **Plot & UI assets are SVG (vector).** They scale cleanly on the canvas at any zoom
  and in printed/PDF tech riders. Rendered via Qt's SVG support.
- Bundle runtime assets through the Qt resource system — see `resources.qrc` at the
  project root. Adding a symbol = drop the SVG in `assets/plot/` and add one `<file>`
  line to `resources.qrc`.
- The plot library starts small (immediate needs) and broadens over time. Keep files
  named clearly and grouped by category as the library grows.
- Documentation images (screenshots, diagrams) live in `docs/images/`, **not** here.