# Research: Concept-to-Scene Pipeline for Modular Kit-Bashing

**Status:** Research / Exploration
**Date:** 2026-02-20

## Problem Statement

I'm building a modular sci-fi environment kit-bashing pipeline where an AI agent (Claude Code) spawns Unreal Engine actors via MCP tools. The kit is ~20 modular pieces (walls, floors, doors, corners, roof panels, skylights) on a 500cm grid.

**Current workflow:** manually place pieces in-editor → verify visually → export as JSON manifest → AI replays the manifest. This works for repetition but not for new designs.

**Target workflow:** share a concept image or text description → AI proposes a spatial layout → spawns a first-pass framework → human tweaks in-editor.

## Key Insight — Preview/Demo Scenes as Training Data

Most Unreal Marketplace asset packs ship with preview levels that demonstrate how pieces fit together. These are artist-authored reference compositions. Using MCP tools (`get_actors_in_level`, `get_actor_details`), we could programmatically scan these demo scenes to extract:

- Every mesh used, its transform, and how it connects to neighbors
- The grid conventions and snap patterns the artist intended
- Common compositions (room types, corridor patterns, transitions)
- The "design grammar" of each kit — which pieces go together and how

This could auto-generate a per-pack template/catalog that an AI can reference when designing new layouts.

## Research Questions

1. What information does an AI need about each modular piece to make spatial/aesthetic decisions? (Dimensions, visual description, connectivity rules, pivot offsets?)
2. What's the minimum viable "mesh catalog" format that bridges visual understanding and programmatic spawning?
3. Can we reverse-engineer preview scenes into reusable templates — scan the demo level, cluster pieces into "room archetypes," and store those as composable building blocks?
4. Are there existing tools/papers on concept-art-to-modular-layout pipelines? (Especially for games using kit-bashing workflows)
5. Could a vision model (Claude, GPT-4V) look at a concept sketch and output a grid-based floorplan with piece assignments, given a good enough kit catalog derived from preview scenes?
6. What are the practical limits — where does AI layout proposal help vs. where does human eye always win?
7. How do PCG systems (like UE5's PCG framework) compare to AI-driven layout for this use case? Can they complement each other?
8. What's the pipeline for onboarding a new asset pack? (Scan preview scene → extract catalog → test with simple spawns → ready for AI-driven composition)

## Asset Packs in Project

- **ModularSciFi** — Primary kit for hab generator. ~20 modular pieces on 500cm grid.
- **Marlitia_Outpost** — Outpost environment assets (preview level available)
- **ScifiCityscape** — Citadel/building assets
- **Chaotic_Skies** — Sky/atmosphere (not spatial)

## Next Steps

- [ ] Scan ModularSciFi preview scene via MCP to extract piece catalog
- [ ] Screenshot each unique mesh piece with dimensions/pivot annotations
- [ ] Define "mesh catalog" JSON format (piece name, dimensions, visual description, connectivity rules)
- [ ] Test: give Claude a catalog + concept description, see if it can output a valid piece manifest
- [ ] Explore whether scanning Marlitia_Outpost demo level yields useful room archetypes
