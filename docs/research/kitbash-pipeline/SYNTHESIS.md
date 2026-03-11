# Research Synthesis: AI-Driven Modular Kit-Bashing Pipeline

**Sources:** Claude (C), Perplexity (P), Gemini/GPT (G) — note: Gemini and GPT produced identical content, treated as one source.

---

## 1. Consensus Findings

These recommendations appear across multiple sources. High confidence.

### 1.1 Five-Layer Mesh Catalog Schema
**Sources: C, P, G**

All three sources converge on a per-piece metadata structure with these layers:

| Layer | Fields | Purpose |
|-------|--------|---------|
| **Identity** | asset_path, display_name, description | Engine spawning + LLM reasoning |
| **Spatial** | dimensions_cm, bounding_box, pivot_offset, grid_snap_interval | Grid-compliant placement |
| **Connectivity** | sockets (position, direction, type label, compatible_with) | Piece-to-piece joining rules |
| **Adjacency** | per-direction allowed neighbors, WFC-compatible rule format | Constraint solving / validation |
| **Semantic** | category, tags, function, style_notes, spawn_weight | LLM piece selection from text/image prompts |

**Most specific version (P):** Includes concrete JSON schema with `grid.size` as cell count, `rotations_deg` whitelist, socket `compatible_with` arrays, `constraints.requires_support_below`, and `stats.frequency_in_examples` derived from demo scenes.

**Key insight (C):** "The LLM consumes the catalog differently than the engine does." The LLM needs `display_name`, `description`, `category`, `function` (natural-language role). The engine needs `asset_path`, `pivot_offset_cm`, exact socket positions. Both consumers must be served from a single source of truth.

### 1.2 Demo Scene Scanning as Primary Data Source
**Sources: C, P, G**

All sources agree that artist-authored preview levels encode a "design grammar" that can be reverse-engineered programmatically. The scanning pipeline has consistent stages across sources:

1. **Enumerate** all actors via MCP `get_actors_in_level`
2. **Filter** to structural kit pieces (exclude lights, cameras, volumes)
3. **Detect grid** via GCD/histogram of position deltas between same-type pieces
4. **Snap** positions to detected grid, quantize rotations to 90deg increments
5. **Build adjacency graph** — nodes=actors, edges=pieces within 1-1.5x grid distance sharing a connection face
6. **Cluster into archetypes** — connected components or DBSCAN, classify by aspect ratio + opening count
7. **Store templates** as relative-transform piece lists with typed sockets at cluster boundaries

**Most specific version (C):** Provides concrete heuristics for archetype classification:
- Aspect ratio >4:1, 2 opposite openings = **corridor**
- 3 openings = **T-junction**, 4 openings = **crossroads**
- Aspect ratio <2:1, larger area = **room**
- 1 opening = **dead end**, 2 openings with 90deg bend = **L-corner**

### 1.3 Layered Hybrid Architecture
**Sources: C, P, G**

All sources describe essentially the same multi-layer pipeline:

1. **VLM/LLM layer** — Interprets concept art or text, outputs abstract layout (room graph with types/sizes/relationships)
2. **Constraint/WFC layer** — Validates and repairs the abstract layout using adjacency rules from demo scene scanning
3. **PCG/detail layer** — Fills structural layout with props, vegetation, lighting
4. **MCP execution layer** — Translates placement data into `spawn_actor` calls

**Key principle (all sources):** "LLM proposes, constraints dispose." Never trust raw LLM spatial output without validation.

### 1.4 LLM Spatial Reasoning Limits
**Sources: C, P, G**

All sources acknowledge VLM spatial reasoning degrades at scale:
- **C (most specific):** Cites "Stuck in the Matrix" paper — 42.7% performance drop (up to 84%) as grid size increases. Models handle 5x5 reasonably, lose coherence at larger scales.
- **P, G:** Acknowledge limits more generally — LLMs struggle with "micro-precise snapping" and coordinate consistency.

**Consensus mitigation:** Target <=10x10 cells per generation pass. Use symbolic/structured representations over natural language. Always post-process with constraint solver.

### 1.5 AI = Blockout Speed, Human = Composition & Polish
**Sources: C, P, G**

| AI handles | Human handles |
|-----------|---------------|
| First-pass structural layout | Overall composition and focal points |
| Grid-compliant placement | Lighting design and mood |
| Connectivity verification | Narrative flow through spaces |
| Variant generation | Hero moment placement |
| Populating repetitive areas | Micro-detail and intentional imperfection |
| Constraint satisfaction | Final aesthetic polish |

**C** cites Promethean AI reporting 10x faster production and 82% of game developers in 2025 GDC survey saying human designers remain essential.
**G** cites industry guidance: "the levels it produces won't be as appealing as if a creative person made them by hand."
**P** frames it as: AI is a "junior blockout designer" giving 60-80% of a plausible layout quickly.

### 1.6 PCG Complements AI, Doesn't Replace It
**Sources: C, P, G**

All agree PCG and AI address different layers:
- **PCG** = deterministic rules, guaranteed geometric consistency, scatter/fill operations
- **AI** = semantic understanding, pattern selection, style variation, high-level decisions

**Best combined architecture (P, C):** AI proposes room graph with parameters (size, density, style). PCG graphs handle internal tiling and prop dressing per room, respecting AI-provided parameters.

### 1.7 Onboarding Pipeline for New Asset Packs
**Sources: C, P, G**

All describe a similar multi-step process:

| Step | Automated? | Time est (C) |
|------|-----------|---------------|
| Import and mesh inventory | Yes | ~5 min |
| Grid/convention detection | Yes | ~10 min |
| Catalog generation (spatial+semantic) | Semi (spatial auto, semantic manual) | ~1 hr |
| Validation spawn tests | Yes | ~30 min |
| AI-ready tagging + AI test | Manual + auto | ~1-2 hrs |

**Total: 2-4 hours per ~20-piece kit** (C). Most time on manual semantic annotation.

**Critical insight (all):** Most connectivity rules can be auto-extracted from demo scenes rather than authored manually.

---

## 2. Contradictions & Tensions

### 2.1 WFC as Validator vs. WFC as Generator
- **C** positions WFC primarily as a **validation/repair layer** — LLM generates layout, WFC fixes violations. Cites CG-WFC hybrid where graph grammar handles macro structure, WFC handles micro placement.
- **P** describes WFC as a **constraint-satisfaction solver** that could be the primary layout engine, with LLM providing the archetype graph and WFC doing the actual tiling.
- **G** barely mentions WFC, focusing on PCG and LLM layout.

**Tension:** Should WFC be downstream of the LLM (validator) or should it be the primary spatial solver fed by LLM-selected constraints? The CG-WFC hybrid (Layer 1 = mission grammar, Layer 2 = spatial WFC) suggests both roles coexist but at different scales.

### 2.2 Socket-Type Matching vs. Tile-ID Matching
- **C** explicitly advocates **socket-type matching** over tile-ID matching for WFC: "a wall's left edge socket can connect to any piece exposing a compatible `wall_edge` socket regardless of the specific mesh."
- **P** also uses socket types but frames them more as categorical tags on tile edges: `wall_continuation`, `doorway`, `corner_cap`, etc.
- **G** mentions "connector faces" and "snap points" but doesn't commit to a specific matching system.

**Tension:** Socket-type matching is more flexible (new pieces auto-integrate if they expose the right socket types) but harder to validate exhaustively. Tile-ID matching is simpler but requires explicit rules for every pair.

### 2.3 Archetype Storage: Tile Pattern vs. Relative Transforms
- **P** stores archetypes as **tile patterns** — 2D arrays of piece IDs that fill a bounding rectangle. Elegant for regular grids.
- **C** stores archetypes as **relative transform lists** — pieces with positions normalized to cluster centroid, more flexible for irregular shapes.

**Tension:** Tile patterns are simpler for WFC integration but can't represent non-rectangular or multi-level archetypes. Relative transforms handle arbitrary shapes but lose the grid-based composability.

**Recommendation:** Use tile patterns for simple rooms/corridors (where WFC operates), relative transforms for complex compositions (L-shapes, multi-story). Our existing hab variation JSONs already use relative transforms.

### 2.4 CLIP/Embedding-Based Matching vs. Text Description
- **G** suggests "a small icon or CLIP embedding could help an AI understand the style" of each piece.
- **C** relies entirely on text descriptions and category tags for LLM reasoning.
- **P** uses `style_tags` and `function_tags` without mentioning embeddings.

**Tension:** CLIP embeddings would enable direct image-to-piece matching but add complexity and a dependency on an embedding model. Text descriptions are simpler and work with any LLM. Given our MCP pipeline and zero-ongoing-cost constraint, text descriptions are more practical.

---

## 3. Unique Findings

### From Claude (C) Only

| Finding | Tag | Notes |
|---------|-----|-------|
| **DeBroglie WFC library** JSON format supports tile symmetry types (X, I, T, L, backslash), adjacency pairs, path constraints, border constraints, frequency multipliers | `[proven]` | DeBroglie is a production C#/.NET WFC library with published documentation |
| **Chain-of-Symbol (CoS) prompting** achieves 60.8% accuracy improvement (31.8%→92.6%) on spatial reasoning benchmarks, 65.8% token reduction | `[proven]` | Published research result |
| **Visualization-of-Thought (VoT) prompting** yields 27% accuracy boost over standard CoT | `[proven]` | Published research result |
| **HouseLLM** (arXiv 2411.12279) uses CoT room-by-room sequential placement + diffusion refinement | `[proven]` | Published paper |
| **ChatHouseDiffusion** (arXiv 2410.11908) shows GPT-4-turbo excels at room type recognition but struggles with sizing | `[proven]` | Published paper |
| **Text-to-Layout** (arXiv 2509.00543) builds full NL→JSON→Revit pipeline | `[proven]` | Published paper |
| **DreamGarden** (CHI 2025 Best Paper) uses LLM-driven planner with hierarchical action plans for UE environment development | `[proven]` | CHI Best Paper, peer-reviewed |
| **Stuck in the Matrix** (arXiv 2510.20198) quantifies LLM grid-reasoning degradation | `[proven]` | Published paper |
| **Matrix Awakens** demo used WFC for city generation | `[proven]` | Epic Games public demo |
| **DBSCAN clustering** (epsilon=1.5x grid, minPts=3-5) for noise-tolerant archetype discovery | `[speculative]` | Logical application of standard algorithm, not tested on modular kits |
| **Hierarchical clustering with average linkage** for multi-scale analysis (walls→rooms→wings→buildings) | `[speculative]` | Sound approach but untested at this scale |
| **Houdini HDAs** as alternative to PCG (Radu Cius's "Procedural Level Design", "Modular Procedural Dungeon") | `[proven]` | Published Houdini tools, but requires $269+/yr license |
| UE5 ships with **experimental built-in WFC plugin** | `[speculative]` | Mentioned but not verified; LebeDevUE's "Wave Function Collapse 3D" on Fab is confirmed commercial alternative |

### From Perplexity (P) Only

| Finding | Tag | Notes |
|---------|-----|-------|
| **Walkable region metadata** per piece (which cells are walkable) for navigation-aware layout | `[speculative]` | Good idea, no existing implementation cited |
| **Clearance metrics** (min ceiling height, door opening size) from level-design standards | `[speculative]` | Standard practice in AAA modular kits (Skyrim/Fallout talks) but not formalized in a schema |
| **Stability rules** ("requires support below", "cannot stack more than N high") | `[speculative]` | Logical extension of constraint system |
| **pcg_presets.json** mapping archetype tags to UE PCG graph parameters | `[speculative]` | Novel integration point between archetype system and PCG, untested |
| **Sketch 2 Graybox** (Stanford CS231A project) converts top-down 2D sketches to graybox levels via Houdini+UE | `[proven]` | Stanford student project, published |
| **Archetype-level AI planning** — AI proposes room graph of archetype nodes + edges, system instantiates each internally | `[speculative]` | Logical architecture, not implemented as described |
| **Flood-fill from door connectors** to discover rooms (areas bounded by walls with few doorway sockets) | `[speculative]` | Intuitive algorithm, not benchmarked |

### From Gemini/GPT (G) Only

| Finding | Tag | Notes |
|---------|-----|-------|
| **LayoutGPT** demonstrates LLM converting text to plausible 2D/3D layouts with numerical spatial constraints | `[proven]` | Published research |
| **ChatDesign** (CAADRIA 2024) uses LLM chat loop to iteratively refine floor plans via locate/move/add/delete operations | `[proven]` | Published paper |
| **Synthetic training data** from demo scenes: render top-down views of example rooms, few-shot/fine-tune VLM to map images to grid instructions | `[speculative]` | Logical idea, not implemented |
| **Scene graph** representation as foundation for template extraction | `[speculative]` | Standard concept but novel application to modular kits |

---

## 4. Organized by Domain

### 4.1 Mesh Catalog Schema

**Required fields per piece (union of all sources):**

```json
{
  "id": "wall_A_1x1",
  "asset_path": "/Game/Kits/SciFi/SM_Wall_A.SM_Wall_A",
  "display_name": "Standard Wall Panel",
  "description": "5m straight wall panel with ribbed metal surface and conduit runs",
  "category": "wall",
  "tags": ["structural", "enclosure", "industrial"],

  "grid": {
    "size_cells": [1, 1, 1],
    "cell_size_cm": 500,
    "dimensions_cm": [500, 500, 500],
    "bounding_box": { "min": [0, 0, 0], "max": [500, 500, 500] },
    "pivot_offset_cm": [0, 0, 0]
  },

  "rotations_deg": [0, 90, 180, 270],

  "sockets": [
    {
      "name": "south_edge",
      "side": "south",
      "offset": 0,
      "type": "wall_continuation",
      "compatible_with": ["wall_continuation", "corner_in", "corner_out", "doorway"]
    }
  ],

  "adjacency_wfc": {
    "+x": ["wall_A_1x1:0", "corner_A:90", "door_A:0"],
    "-x": ["wall_A_1x1:0", "corner_A:270"],
    "+y": [],
    "-y": []
  },

  "semantics": {
    "function": "Enclosure wall for rooms and corridors",
    "style_tags": ["industrial", "panelled", "medium_wear"],
    "spawn_weight": 0.21,
    "typical_neighbors": ["corner_A", "pillar_A", "doorframe_A"]
  },

  "constraints": {
    "requires_support_below": true,
    "exterior_only": false,
    "context_rules": ["Only on room perimeter"]
  }
}
```

**What the LLM needs:** `display_name`, `description`, `category`, `function`, `dimensions_cm`, `sockets[].type`, `semantics.*`
**What the engine needs:** `asset_path`, `pivot_offset_cm`, `sockets[].name` + position, `grid.cell_size_cm`

### 4.2 Demo Scene Scanning

**Algorithm (synthesized from all sources):**

1. `get_actors_in_level` → filter to StaticMeshActors with kit meshes
2. Grid detection: GCD of position deltas OR histogram mode of inter-actor distances
3. Snap positions to grid, quantize rotations to 90deg increments
4. Adjacency graph: edge-adjacent footprints + compatible facing sockets
5. Clustering:
   - **Connected components** (C, P, G) — simplest, works for clearly separated rooms
   - **DBSCAN** (C only, epsilon=1.5x grid, minPts=3-5) — handles noise, arbitrary shapes
   - **Flood-fill from doors** (P only) — semantically motivated, finds "rooms" bounded by walls
   - **Hierarchical** (C only) — multi-scale (walls→rooms→wings)
6. Archetype classification heuristics (C):
   - aspect_ratio > 4:1, 2 openings → corridor
   - 3 openings → T-junction
   - 4 openings → crossroads
   - aspect_ratio < 2:1, large area → room
   - 1 opening → dead end
7. Store as relative-transform templates with boundary sockets

**MCP tools needed:** `get_actors_in_level`, `get_actor_details` (for mesh path + bounding box), `find_actors_by_name` (for filtering)

### 4.3 Layout Generation

**Best prompting strategies (from C):**
- **Chain-of-Symbol (CoS):** Condensed symbolic spatial representations → 60.8% accuracy improvement
- **Room-by-room sequential:** Place one room at a time with explicit coordinate tracking (more reliable than whole-layout generation)
- **Few-shot with kit examples:** Provide catalog + 2-3 example layouts from scanned templates
- **Two-phase for concept art:** Phase 1 = VLM extracts semantic description (room types, relationships, proportions). Phase 2 = LLM converts to grid coordinates using catalog

**Grid size constraint:** <=10x10 cells per generation pass (all sources). For larger layouts, generate in chunks and stitch.

**Academic references:**
- HouseLLM — CoT room-by-room + diffusion refinement
- ChatHouseDiffusion — GPT-4-turbo for room recognition, weak on sizing
- ChatDesign — Iterative LLM loop with locate/move/add/delete operations
- LayoutGPT — Text-to-2D/3D layout with numerical constraints
- Text-to-Layout — NL→JSON→Revit full pipeline
- DreamGarden — LLM planner + UE environment modules (CHI 2025 Best Paper)

### 4.4 Constraint Solving

**WFC libraries:**
| Library | Language | Status | Notes |
|---------|----------|--------|-------|
| **DeBroglie** | C#/.NET | Production | Best documented JSON format, supports symmetry types, path constraints |
| **mxgmn reference** | C# | Reference impl | Original WFC, not designed for production use |
| **UE5 built-in WFC** | C++ | Experimental | Unverified, may not exist in 5.7 |
| **LebeDevUE WFC 3D** | C++/BP | Commercial (Fab) | Paid plugin, UE5 native |

**How adjacency rules feed WFC (C):**
- Demo scene scanning extracts observed adjacencies
- Convert to DeBroglie-format JSON: tile IDs, rotation variants, adjacency pairs per direction
- Socket-type matching (not tile-ID matching) for flexibility
- Frequency multipliers from observed occurrence rates in demo scenes

**WFC roles in pipeline:**
- **Validator:** LLM proposes layout → WFC checks all adjacencies → flags violations
- **Repairer:** WFC re-collapses violated cells while preserving valid placements
- **Generator:** For detail-filling (which wall variant goes where) within LLM-defined room boundaries

### 4.5 PCG Integration

**Where PCG fits (all sources agree):**
- NOT for macro layout (room graph, corridor routing) — that's AI's job
- YES for:
  - Internal tiling within AI-defined room volumes
  - Prop scatter (vegetation, debris, small objects)
  - Detail dressing (pipes, cables, decals)
  - Parametric variation (density, style) controlled by AI-provided tags

**UE5 PCG limitations for modular snapping:** No built-in adjacency/socket matching. Custom nodes needed. GPU execution in 5.7 provides ~2x speedup for scatter operations.

**Integration point (P):** `pcg_presets.json` mapping archetype tags → PCG graph parameters. AI selects archetype, PCG graph receives parameters and handles detail placement.

### 4.6 Onboarding Pipeline

**Synthesized 6-step process:**

| Step | Action | Automated | Time |
|------|--------|-----------|------|
| 1 | Import pack, open demo level | Manual | 5 min |
| 2 | Scan all meshes: paths, bounds, pivots, materials | Auto (build_catalog.py) | 5-10 min |
| 3 | Detect grid (GCD of position deltas in demo scene) | Auto (new tool) | 5 min |
| 4 | Build adjacency graph from demo scene | Auto (new tool) | 10 min |
| 5 | Cluster + classify archetypes | Auto (new tool) | 10 min |
| 6 | Add semantic annotations (descriptions, function tags) | Manual | 1-2 hrs |
| 7 | Spawn test: replay archetypes in empty level | Auto (new tool) | 30 min |
| 8 | AI generation test: prompt LLM with catalog + description | Semi-auto | 30 min |

**Critical insight:** Steps 3-5 replace most manual rule authoring. The demo scene IS the training data.

### 4.7 Academic & Industry References

| Reference | Type | Source | URL/ID |
|-----------|------|--------|--------|
| HouseLLM | Paper | C | arXiv 2411.12279 |
| ChatHouseDiffusion | Paper | C | arXiv 2410.11908 |
| Chat2Layout | Paper | C | — |
| Text-to-Layout | Paper | C | arXiv 2509.00543 |
| DreamGarden | Paper (CHI 2025 Best) | C | Sam Earle et al. |
| Stuck in the Matrix | Paper | C | arXiv 2510.20198 |
| Chain-of-Symbol | Paper | C | — |
| Visualization-of-Thought | Paper | C | — |
| LayoutGPT | Paper | G | — |
| ChatDesign | Paper (CAADRIA 2024) | G | — |
| Sketch 2 Graybox | Project (Stanford) | P | CS231A |
| DeBroglie | WFC Library | C | github.com/BorisTheBrave/DeBroglie |
| Dungeon Architect | UE Plugin | C, P | UE Marketplace |
| Promethean AI | Commercial Tool | C | prometheanai.com |
| Bad North | Game (WFC terrain) | C | — |
| Caves of Qud | Game (WFC ruins) | C | — |
| Townscaper | Game (WFC cities) | C | — |
| Matrix Awakens | Epic Demo (WFC city) | C | — |
| Joel Burgess GDC talks | Industry (Skyrim/FO4) | C | — |
| Morai Maker study | Paper (ACM CHI 2019) | C | — |
| Radu Cius Procedural Level Design | Houdini HDA | C | — |
| LebeDevUE WFC 3D | UE Plugin (Fab) | C | — |
