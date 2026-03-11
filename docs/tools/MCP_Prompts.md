# MCP Prompt Reference

A collection of detailed prompts for use with the Blender MCP and Unreal MCP tools. The more detail you give in the prompt, the closer the result will be to what you want.

---

## Blender MCP Prompts

### Props & Objects

- "Create a sci-fi supply crate — 1m cube, dark metal material with scratches, glowing blue trim along the edges, a handle on top, and hazard stripes on one side"
- "Build a medieval wooden barrel with iron bands, slightly warped staves, a cork in the top, and a worn wood material with high roughness"
- "Make a futuristic holographic terminal — a thin rectangular base with a translucent blue emissive screen floating above it at an angle, with a keyboard panel on the base"
- "Create a set of 5 potion bottles in different shapes — round, tall, flask, vial, and jug — all with glass material and different colored emissive liquids inside"

### Environment Pieces

- "Build a modular sci-fi corridor section — 4m wide, 3m tall, 6m long, with recessed lighting panels in the ceiling, conduit pipes along the walls, and a metal grate floor. Use a dark gunmetal material with orange accent lights"
- "Create a ruined stone archway — broken on the right side, with ivy-like protrusions on the remaining stones, moss-colored patches on the base, and rubble pieces scattered at the ground level"
- "Make a sci-fi door frame with sliding door panels — the frame should have warning stripes, the doors should be two halves that would slide apart, with a control panel on the right side of the frame"
- "Build a modular staircase kit — straight section, 90-degree turn section, and a landing platform, all with metal railings and grated steps, in an industrial style"

### Characters & Creatures

- "Create a low-poly robot companion — boxy torso, cylindrical limbs, a dome head with a single large glowing blue eye, antenna on top, small hover-disc feet instead of legs, and a metallic grey body with yellow caution tape decals"
- "Make a stylized alien plant — a thick trunk splitting into 3 branches, each ending in a large bioluminescent bulb. The trunk should be dark purple, the bulbs should glow cyan with emission, and add small leaf-like fins along the branches"
- "Build a low-poly wolf in a standing alert pose — pointed ears, elongated snout, bushy tail, four legs with simple paw shapes, using a grey fur material with white chest and dark back"

### Vehicles & Machines

- "Create a hover-bike — elongated body like a jet ski, two circular thruster pods on the sides angled downward, handlebars at the front, a seat in the middle, and a small cargo rack at the back. Use a matte black body with neon green thruster glow"
- "Build a simple mining drill machine — a tracked base, a cabin with a windshield on top, and a large spiral drill bit extending forward. Industrial yellow body with metal drill"

### Weapons & Tools

- "Make a sci-fi energy sword — cylindrical handle with grip texture, a cross-guard with small emissive nodes, and a blade made of a bright white-blue emissive material that tapers to a point, with a subtle glow around it"
- "Create a medieval shield and sword set — a kite shield with a raised border and a central emblem bump, and a broadsword with a wrapped handle, disc pommel, and straight cross-guard. Both in brushed steel with leather-brown grip"

---

## Unreal MCP Prompts

### Towns & Cities

- "Create a medium-sized medieval town with a street grid, mix of wooden and stone buildings, street lights, a central plaza with a fountain area, sidewalks, and some market stalls along the main road"
- "Build a futuristic city block — tall skyscrapers, wide streets with lane markings, traffic lights at intersections, urban furniture like benches and planters, and street-level shop fronts"
- "Create a small fishing village — 6 modest houses clustered near a waterfront, with a dock-like platform extending outward, crate props near the dock, and a winding path connecting the houses"

### Castles & Fortresses

- "Build a large medieval castle fortress with thick outer walls, 4 corner towers, a gatehouse with a drawbridge, an inner keep with a courtyard, siege weapons on the walls, and a small village settlement outside the walls"
- "Create a ruined castle — use the castle fortress tool but make it a small size, then add scattered wall segments and towers at odd angles nearby to suggest collapsed sections"

### Houses & Mansions

- "Construct a two-story Victorian mansion with a grand entrance, east and west wings, a porch wrapping around the front, and balconies on the second floor. Make it large scale"
- "Build a row of 4 small houses along a street — vary the styles between cottage and colonial, space them 2000 units apart, and add a wall segment between each as a fence"

### Mazes & Puzzles

- "Create a 20x20 maze with tall walls that a first-person character couldn't see over. Place it at the center of the map. Then add a tower next to it so a player could climb up and see the maze layout from above"
- "Build a smaller 8x8 maze with low walls, then surround it with a circular wall enclosure with an arch as the entrance"

### Mixed Structures

- "Create a castle courtyard scene — build walls in a square 5000 units wide, add a tower at each corner 800 units tall, put a staircase inside one tower, an arch as the main gate, and a pyramid in the center of the courtyard as a monument"
- "Build a bridge connecting two towers — place two towers 8000 units apart, then create a suspension bridge between them. Add walls extending from each tower to create a walled pathway leading to the bridge on both sides"
- "Create an aqueduct running through a town — build the town first, then place an aqueduct that runs through the middle of it, elevated above the buildings"

### Blueprint Programming

- "Create a Blueprint called BP_HealthPickup with a StaticMesh component (sphere), a collision box component, and variables for HealAmount (float, default 25), bIsActive (boolean, default true), and RespawnTime (float, default 10). Add a print node that says 'Health Picked Up' connected to an event"
- "Create a Blueprint called BP_DayNightCycle with a float variable TimeOfDay that ranges from 0 to 24, a boolean bIsDaytime, and a function called UpdateLighting with a float input for CurrentTime and a boolean output for IsDaytime"
- "Analyze my BP_MainCharacter Blueprint — tell me what variables it has, what functions are defined, and walk me through the event graph logic"

### Level Layout

- "Set up a basic test level — create a large flat wall as a floor (10000x10000), add 4 walls around the edges to keep the player in, place a pyramid at one end as a landmark, a staircase in the corner leading up to a platform made of another wall segment, and a maze in the center"
