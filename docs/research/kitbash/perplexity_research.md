1. What the AI needs per modular piece
For layout decisions, the AI does not need full mesh geometry; it needs a compact, grid-aligned, socket-based abstraction plus some semantic hints.

Geometric / spatial metadata

Grid footprint: width, depth, height in grid cells (e.g. 1×2×1 at 500 cm). This is the primary unit the planner operates on.
​

Bounding box and pivot: local bounds, pivot offset relative to a canonical cell origin (so AI can reason in grid, executor converts to world transform).

Allowed rotations: discrete rotations that preserve snapping (e.g. {0°, 90°, 180°, 270°}).

Walkable region: whether the top surface is walkable, and if so, which cells; needed for navigation-aware layout.

Clearance metrics: min ceiling height above floor tiles, door opening size, etc., derived from level-design metrics.
​

Connectivity / topology

Represent this as sockets on the edges of the tile’s footprint:

Socket list: each socket has:

Side (north/south/east/west/top/bottom)

Offset along that side in cells

Type (wall_continuation, doorway, corner_cap, window, roof_edge, etc.)

Orientation (for asymmetrical pieces)

Compatibility rules:

Which socket types can connect (doorway ↔ doorway, wall_end ↔ wall_end, corridor_T ↔ corridor_T, etc.).

Hard constraints (this socket must not be adjacent to anything; this must border void/sky).

This is close to how modular kits are recommended to be designed for robust composition in AAA workflows (Skyrim/Fallout modular talks, etc.).
​

Semantic / visual metadata

You only need coarse tags so the AI can match concept art or text prompts:

Category: wall, floor, door, corner, roof, skylight, pillar, trim, prop.

Style tags: “industrial”, “clean”, “grungy”, “high-tech”, “glass-heavy”, “open/closed”, “bright/dark”.

Function tags: “main corridor”, “secondary corridor”, “junction”, “entrance”, “windowed wall”, “cover object”, “focal piece”.

Repetition priors: typical usage frequency (common/rare), per-archetype weights.

Usage and constraints

Context rules: “only on exterior perimeter”, “only above floor tiles”, “only under roof tiles”, “must attach to doorframe” etc.

Stability rules: “requires support under this footprint cell”, “cannot be stacked more than N high”, etc.

Pack-specific exceptions: “this piece only appears in hangar archetypes”, etc., which you can infer from preview scenes.

This is the level of abstraction most modular-kit best practices implicitly target: clear metrics, fixed footprints, standardized module types, and naming that encodes intent.
​

2. Minimum viable mesh catalog format
You want a catalog that:

Is grid-/socket-based so an algorithm can compose layouts without touching raw transforms.

Has enough semantic descriptors to map from concept art / text prompts.

Is simple enough to be generated automatically from the pack + preview levels with light human annotation.

A practical JSON (or protobuf) schema per piece might look like:

text
{
  "id": "wall_A_1x1",
  "asset_path": "/Game/Packs/SciFi/SM_Wall_A_1x1",
  "category": "wall",
  "grid": {
    "size": [1, 1, 1],
    "cell_size_cm": 500,
    "pivot_offset_cm": [0, 0, 250]
  },
  "rotations_deg": [0, 90, 180, 270],
  "sockets": [
    { "name": "north_wall", "side": "north", "offset": 0, "type": "wall_continuation", "compatible_with": ["wall_continuation", "corner_in", "corner_out"] },
    { "name": "south_wall", "side": "south", "offset": 0, "type": "wall_continuation", "compatible_with": ["wall_continuation", "corner_in", "corner_out"] }
  ],
  "semantics": {
    "style_tags": ["industrial", "panelled", "medium_wear"],
    "function_tags": ["enclosure"],
    "visual_notes": "Flat panel wall with recessed tech panels and dim strip lighting."
  },
  "constraints": {
    "allowed_levels": ["ground", "upper"],
    "requires_support_below": true,
    "exterior_only": false
  },
  "stats": {
    "frequency_in_examples": 0.21,
    "typical_neighbors": ["corner_A", "pillar_A", "doorframe_A"]
  }
}
Catalog-level content:

pieces[]: all entries like above.

archetypes[]: room/corridor templates (next section).

metrics: global grid size, preferred corridor width in cells, min/max room sizes, etc., aligned with standard modular-metric guidelines.

Your AI planner then operates only in this abstract space:

From concept → produce: “10×6 main hall, 3× corridor branches, windows on north wall, skylights in center”, expressed as a grid + high-level tags.

From high-level grid + tags → solve for a tiling of these catalog pieces that satisfies socket compatibility, constraints, and frequency priors.

Executor (MCP tool) converts grid cell + rotation + piece ID → UE actor transform on the 500 cm grid.

3. Reverse-engineering preview scenes into templates
Your idea of mining preview levels is strong: they encode the pack’s “design grammar” for free.

Step 1: Extract and normalize

From each preview/demo level:

Use MCP tools to list all StaticMeshActors (or Blueprints that wrap them), recording: mesh asset, world transform, level name.

Snap world positions to the nearest grid cell and infer the grid size by taking the GCD or histogram mode of deltas between neighboring actors.

Normalize coordinates to local pack-space (e.g., origin at bottom-left of the used bounding box).

Step 2: Build connectivity graph

For each placed piece, determine adjacency:

Two actors are neighbors if:

Their grid footprints are edge-adjacent, and

Their facing and socket types are compatible (based on preliminary auto-detected sockets or naming conventions).

Represent the level as a graph:

Nodes: piece instances.

Edges: valid connections (door-to-door, wall-to-wall, etc.).

Also compute higher-level features: connected components, degree of nodes, loops vs dead-ends.

This graph-grammar style representation is similar in spirit to systems like Dungeon Architect, which stitch rooms via connectors and then use PCG systems for detail.
​

Step 3: Discover room and corridor archetypes

Use spatial clustering to cut the graph into modules:

Start from door connectors and flood-fill to find “rooms” (areas bounded by walls with a small number of doorway sockets).

Identify corridor segments as long narrow components (aspect ratio > some threshold, consistent width).

For each component, derive a feature vector:

Bounding box in grid cells (width, height).

Number and orientation of entrances/exits.

Distribution of piece categories (walls vs doors vs props).

Style summary (majority of style tags).

Cluster these using something simple (k-means/DBSCAN) to obtain archetypes:

“3×5 single-exit dead-end room”

“2-wide straight corridor, 5 long”

“4-way junction room with central pillar”

Step 4: Store archetypes as composable templates

Each archetype becomes an entry like:

text
{
  "id": "room_3x5_single_door_A",
  "grid_size": [3, 5],
  "entrances": [
    { "side": "south", "offset": 1, "socket_type": "doorway" }
  ],
  "tile_pattern": [
    ["wall_A", "wall_A", "wall_A"],
    ["wall_A", "floor_A", "wall_A"],
    ["wall_A", "floor_A", "wall_A"],
    ["wall_A", "floor_A", "wall_A"],
    ["wall_A", "door_A", "wall_A"]
  ],
  "style_tags": ["industrial", "corridor_like"],
  "source_examples": [
    { "level": "Preview_L01", "origin_cell": [10, 4] }
  ]
}
Now your AI planner can operate at the archetype graph level:

Compose layouts as a graph of room/corridor archetypes connected by entrance sockets.

Then instantiate each archetype internally using its stored pattern.

4. Existing tools/papers for concept → modular layout
There is related work, though nothing exactly matches “mine preview scenes for grammar then use LLM + MCP” (which is your novel twist).

Relevant examples:

Sketch 2 Graybox (Stanford CS231A project):
Takes top-down 2D sketches and automatically generates graybox levels (walls, floors, and instance points) in Unreal via Houdini Engine.
​

They use edge detection, contour finding, and sketch symbol classification, then generate 3D geometry and transform gameplay actors to instance points.

This is very close to your “sketch → grid layout → UE instancing” idea, but using Houdini instead of an LLM.

AI tools that convert sketches to floor plans:
Several tools and services take hand-drawn floorplans and convert them to clean floorplans or 3D walkthroughs.
​
Conceptually, they are performing a similar mapping from rough visual intent to structured layout primitives.

Modular kit design / level design literature:
Guides like The Level Design Book’s modular-kit section describe metrics, module types, and best practices for modular level construction.

AI + PCG overviews:
Articles on combining AI with PCG highlight using ML to guide or parameterize rule-based generators, especially in engines like Unreal with PCG frameworks.

These show that: (a) sketch-to-graybox is practical, and (b) combining learned guidance with PCG in Unreal is already a recognized direction. Your contribution is “kit-specific learned grammar derived from preview scenes” plus LLM-based authoring.

5. Vision model → grid floorplan with piece assignments
With a robust catalog + archetypes, a modern vision model (Claude, GPT-4V, etc.) can plausibly go from concept art/sketch to a grid layout specification, then a solver maps that to actual kit pieces.

Recommended two-stage approach

Concept → abstract layout

Input: concept art (ideally top-down / orthographic / blocky paintover) or a markup sketch.

Ask the vision model to output:

A coarse grid (e.g. 1 cell = 500 cm) with labeled cells: wall, door, window, floor, void, stair, pillar, skylight, etc.

A list of rooms with approximate sizes and adjacency (graph or simple table).

Tags per area: “hangar-like”, “narrow corridor”, “control room”, “open atrium”.

This is conceptually identical to Sketch 2 Graybox, which extracts walls/floors from a sketch then builds 3D geometry in UE.
​

Abstract layout → kit assignment

A constraint-satisfaction / search layer (could be partially LLM-driven) uses:

Your room archetype library, trying to tile the abstract layout with known archetypes.

Piece-level connectivity constraints and priors from the catalog.

Output: a grid of concrete piece IDs + rotations, guaranteed to be compatible and spawnable.

Why split it?

Vision models are good at coarse spatial reasoning and reading sketches; they are poor at micro-precise snapping and respecting low-level constraints.

A separate symbolic solver ensures you never get half-tile overlaps, invalid doors, or broken ceilings.

In practice, you could represent the intermediate as a human-readable text format (ASCII map or compact JSON) for easy debugging and in-editor overlays.

6. Practical limits: where AI helps vs where humans win
Where AI layout helps

Rapid blockout: Generating multiple graybox variants in minutes for a single concept, especially for corridors, room networks, basic circulation, and cover placement. PCG and AI-driven layouts already show large speedups in grayboxing workflows.
​
​

Grammar consistency: Enforcing kit metrics (grid alignment, clearances, consistent module usage) more strictly and consistently than rushed hand placement.

Exploration: Proposing alternative flows (branching vs linear, different encounter pacing) you might not sketch yourself.

Where human eye still wins

Cinematics and composition: Framing vistas, hero shots, subtle silhouettes, and focal points; this remains hard for automated systems, and human composition knowledge is critical.
​

Gameplay nuance: Micro cover placement, line-of-sight tuning, combat readability, and difficulty curves require iteration with playtests and intention that AI cannot fully internalize.

Narrative/signature spaces: Story beats, environmental storytelling, and unique set dressing almost always need bespoke human intervention.

In practice, treat AI layout as:

“Junior blockout designer” that gives you 60–80% of a plausible layout quickly.

You then do a curation pass: accept, nudge, or discard proposals, then refine hero spaces manually.

7. UE5 PCG vs AI-driven layout, and how they complement
Think of UE5’s PCG framework as a rule-based instancer, and the AI as a planner.

High-level comparison
Aspect	UE5 PCG graphs	AI-driven layout (LLM + solver)	Combined usage
Primary input	Volumes, splines, markers, parameters	Concept image/text → abstract layout spec	AI outputs markers/graphs consumed by PCG
Nature	Deterministic rules & node graphs	Probabilistic, data/grammar-driven decisions	Rule-driven instancing of AI’s abstract plan
Authoring effort	Initial graph setup can be heavy, but reusable
​	Prompt design + catalog/templates; less visual graph work	More effort up front, higher leverage later
Control	Very precise, but rule-centric	Very high-level, risk of surprises	Rules guarantee validity; AI just chooses patterns
Debuggability	Strong (graph stepping, preview)	Weaker (trace through text reasoning or logs)	Use PCG for low-level; AI only at room/graph level
Best at	Filling volumes, scattering, repetitive structures
​	Macro layout, pattern selection, style-based variation	AI picks archetypes; PCG fills details & dressing
PCG is already used in Unreal for fast level prototyping—defining rules to populate volumes and swap prototype assets with final art later. AI can sit one layer above:
​

Example combined architecture

AI proposes:

Room graph (nodes = archetypes, edges = doors).

For each node, tags and parameters (size, density, “clutter level”, “industrial vs clean”).

The system instantiates a PCG volume or Level Instance per node.
​

Within each volume:

PCG graphs handle the detailed tiling and/or clutter of that room, respecting parameters from AI (density, style variants, prop sets).

PCG also handles secondary dressing—pipes, cables, decals, small props—where you don’t want the AI to reason explicitly.

This mirrors existing workflows where grammar-based systems like Dungeon Architect define room graphs and use PCG/external generators for local geometry; one Reddit example even connects Dungeon Architect rooms with PCG-generated splines.
​

8. Pipeline for onboarding a new asset pack
Given your MCP tool access, you can standardize pack onboarding as a semi-automatic offline process.

Step 0: Conventions and assumptions

Assume or infer a base grid (e.g. 500 cm). If the pack uses a different grid, detect it by analyzing repeated offsets in the preview map.

Define a minimal naming/metadata convention you’ll rely on when auto-extracting semantics (e.g., “Wall”, “Door”, “Floor” in mesh names).

Step 1: Mesh scan → raw catalog

Iterate over all Static Mesh assets in the pack:

Extract bounds, pivot, collision, LOD info, materials.

Quantize bounds to grid cells to estimate grid footprint.

Heuristically classify category via name and aspect ratio:

Long, thin, vertical → wall

Flat, horizontal, large → floor/roof

Has opening in middle → door/window

Store an initial catalog entry per mesh (geometry + best-guess category).

Optionally, build an in-editor UI to let a human quickly correct misclassifications and tag style/function; this is cheap and pays off downstream.

Step 2: Auto-detect sockets and constraints

For each mesh:

Sample the footprint per side; where collision/geometry “touches” the side, consider a candidate socket.

Refine with naming patterns (“End”, “Corner”, “Door”) and how the mesh appears in preview levels (what it tends to connect to).

From this, generate:

Socket definitions (side/offset/type).

Basic compatibility heuristics (wall_with_door often connects to corridor pieces, etc.).

This does not need to be perfect; your archetype mining (next step) will refine priors.

Step 3: Preview level mining → archetypes & priors

For each included demo/preview level:

Extract actor placements, snap to grid.

Build adjacency graph and segment into rooms/corridors as described earlier.

Cluster to derive archetypes and compute statistics:

How often each piece appears.

Typical neighbors for each piece.

Common room shapes and entry configurations.

Attach these stats to the catalog:

frequency_in_examples, typical_neighbors, archetype_memberships.

This approximates the “design grammar” the original artist used when building the preview. It’s similar in spirit to how modular kits are stress-tested and iterated by building test layouts.
​

Step 4: Serialize pack template

Emit:

mesh_catalog.json (or similar) with all piece definitions.

archetypes.json with room/corridor templates.

Optional pcg_presets.json that map archetype tags to particular UE PCG graphs or parameters.

Version this per pack (and per pack version), so your AI always knows exactly which catalog it is targeting.

Step 5: Simple spawn tests

Before trusting it in AI-driven generation:

Use a small “sanity” tool (another MCP action) that:

Reads the catalog and archetypes.

Randomly spawns a few archetype instances in an empty test level on the grid.

Visually verify:

Snapping is correct (no seams or overlaps).

Doors line up; no impossible geometry.

Style tags roughly match visual reality.

Iterate on any meshes/archetypes with problems (fix pivot, tweak sockets, or blacklist problematic pieces).

Step 6: Enable AI-driven composition

Once a pack passes tests, mark it as “AI-ready”:

The AI now knows:

grid_size, metrics, archetypes, and piece constraints.

It can propose new layouts in terms of:

Archetype graph (macro level).

Optional overrides (e.g. “use skylight-heavy roof variants”).

Your MCP tools:

Convert AI’s abstract layout into actor spawns.

Optionally hand off internal tiling/dressing to PCG.

At this point, concept-art → AI layout → UE spawn becomes just another pipeline stage, much like Sketch 2 Graybox does for sketches via Houdini + Unreal.
​

