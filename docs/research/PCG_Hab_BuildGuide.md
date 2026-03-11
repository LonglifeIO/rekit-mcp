# PCG Hab Generator — Step-by-Step Build Guide (UE5.7)

> **Phase 1: Single Parametric Hab**
> Follow each step in order. Every step ends with a test checkpoint — verify before moving on.
> This guide uses **correct UE5.7 node names** verified against the engine.

---

## UE5.7 PCG Node Name Reference

The PCG Graph editor's right-click search uses these names. The old guide used different names — **use the names below**:

| What You Need | Search For (right-click menu) | Settings Class |
|--------------|------------------------------|----------------|
| Point grid generator | `Create Points Grid` | PCGCreatePointsGridSettings |
| Move/rotate points | `Transform Points` | PCGTransformPointsSettings |
| Math on attributes | `Metadata Maths` | PCGMetadataMathsSettings |
| Create a constant/attribute | `Create Attribute Set` | PCGCreateAttributeSetSettings |
| Filter by attribute value | `Filter By Attribute` | PCGFilterByAttributeSettings |
| Read a graph parameter | `User Parameter Get` or `Get Graph Parameter` | PCGUserParameterGetSettings |
| Spawn static meshes | `Static Mesh Spawner` | PCGStaticMeshSpawnerSettings |
| Merge point streams | `Union` | PCGUnionSettings |
| Random density values | `Density Noise` | PCGDensityNoiseSettings |
| Filter by density range | `Density Filter` | PCGDensityFilterSettings |

---

## Step 0: Project Setup

### 0.1 Create Folder Structure

1. In Content Browser, navigate to `/Content/Custom/`
2. Right-click > **New Folder** > name it `PCG`
3. Inside `PCG/`, create another folder > `HabGenerator`
4. Final path: `/Content/Custom/PCG/HabGenerator/`

### 0.2 Create the Enum: `E_HabDoorSide`

1. In `/Content/Custom/PCG/HabGenerator/`, right-click > **Blueprints > Enumeration**
2. Name it `E_HabDoorSide`
3. Open it. Add 4 enumerators (use the **New** button for each):

| Index | Display Name | Description |
|-------|-------------|-------------|
| 0 | Front | +Y side (south edge in grid) |
| 1 | Back | -Y side (north edge in grid) |
| 2 | Left | -X side (west edge in grid) |
| 3 | Right | +X side (east edge in grid) |

4. Save and close.

### 0.3 Create the PCG Graph: `PCG_Hab_Main`

1. In `/Content/Custom/PCG/HabGenerator/`, right-click > **PCG > PCG Graph**
2. Name it `PCG_Hab_Main`
3. Open it. You'll see a default **Input** node on the left and an **Output** node.
4. **Add Graph Parameters** — click the green **Graph Parameters** button in the top toolbar:

| Name | Type | Default Value |
|------|------|---------------|
| `HabLength` | Int32 | 3 |
| `HabWidth` | Int32 | 2 |
| `WindowDensity` | Double | 0.3 |
| `DoorCount` | Int32 | 1 |
| `DoorSideIndex` | Int32 | 0 |
| `HasRoof` | Bool | True |
| `Seed` | Int32 | 12345 |

   To add each: click **+** in the Graph Parameters panel > choose the type > name it > set the default.

5. Save and close for now.

### 0.4 Create the Blueprint Actor: `BP_PCG_Hab`

1. In `/Content/Custom/PCG/HabGenerator/`, right-click > **Blueprint Class > Actor**
2. Name it `BP_PCG_Hab`
3. Open it in the Blueprint editor.

#### 0.4a Add the PCG Component

1. In the **Components** panel (top-left), click **Add** > search for **PCG** > select **PCG Component**
2. Select the PCG Component in the Components list
3. In the **Details** panel on the right:
   - **Graph** > set to `PCG_Hab_Main` (click the dropdown, search for it)
   - **Generation Trigger** > set to `Generate On Demand` (we'll call Generate from the Construction Script)

#### 0.4b Add Instance-Editable Variables

1. Go to the **My Blueprint** panel (bottom-left)
2. Click the **+** next to **Variables** to add each one:

| Variable Name | Type | Default | Instance Editable | Tooltip |
|--------------|------|---------|-------------------|---------|
| `HabLength` | Integer | 3 | Yes | Modules along X (1-6) |
| `HabWidth` | Integer | 2 | Yes | Modules along Y (1-3) |
| `WindowDensity` | Float | 0.3 | Yes | Window probability (0-1) |
| `DoorCount` | Integer | 1 | Yes | Number of door segments (1-4) |
| `DoorSide` | E_HabDoorSide | Front | Yes | Which wall gets doors |
| `HasRoof` | Boolean | True | Yes | Toggle roof generation |
| `Seed` | Integer | 12345 | Yes | Random seed |

**For each variable:**
1. Create the variable (click **+** next to Variables)
2. Click on it in the variable list
3. In the Details panel on the right:
   - Check the **eye icon** next to the variable name (this is Instance Editable)
   - Or find **Instance Editable** checkbox and enable it
4. **Compile** the Blueprint (toolbar button) so defaults can be set
5. Set the default value in the Details panel under **Default Value**
6. For numeric variables, expand the **Advanced** section and set:
   - `HabLength`: Slider Range 1-6, Value Range 1-6
   - `HabWidth`: Slider Range 1-3, Value Range 1-3
   - `WindowDensity`: Slider Range 0-1, Value Range 0-1
   - `DoorCount`: Slider Range 1-4, Value Range 1-4

> **Note:** `DoorSide` uses the `E_HabDoorSide` enum type. When creating this variable, change the type dropdown from "Boolean" to `E_HabDoorSide` (search for it in the type picker).

#### 0.4c Build the Construction Script

Open the **Construction Script** tab (next to Event Graph tab).

**What we're building:** A chain of `Set Graph Parameter Local` nodes that push each Blueprint variable value into the PCG graph parameters, then calls `Generate(Force)`.

**Node-by-node instructions:**

1. **Get the PCG Component reference:**
   - In the Components panel, **drag** the PCG Component onto the Construction Script graph
   - This creates a "Get PCG Component" reference node (blue node)

2. **First parameter — HabLength:**
   - Drag off the **Construction Script** exec pin (white arrow)
   - Search for `Set Graph Parameter Local` — you'll see typed versions
   - Select **Set Graph Parameter Local (Int32)**
   - On this node:
     - **Target** pin: drag a wire FROM the PCG Component reference TO the Target pin
     - **Name** pin: type `HabLength` (must match the PCG graph parameter name EXACTLY)
     - **Value** pin: from the My Blueprint panel, **drag** the `HabLength` variable onto the Value pin

3. **Repeat for each parameter** — chain the exec pins left to right:

| Order | Node Type | Name (string) | Value (drag variable) |
|-------|-----------|---------------|----------------------|
| 1 | Set Graph Parameter Local (Int32) | `HabLength` | HabLength variable |
| 2 | Set Graph Parameter Local (Int32) | `HabWidth` | HabWidth variable |
| 3 | Set Graph Parameter Local (Double) | `WindowDensity` | WindowDensity variable |
| 4 | Set Graph Parameter Local (Int32) | `DoorCount` | DoorCount variable |
| 5 | Set Graph Parameter Local (Int32) | `DoorSideIndex` | *(see step 4 below)* |
| 6 | Set Graph Parameter Local (Bool) | `HasRoof` | HasRoof variable |
| 7 | Set Graph Parameter Local (Int32) | `Seed` | Seed variable |

4. **DoorSide to DoorSideIndex conversion** (for node #5):
   - Drag the `DoorSide` variable onto the graph (creates a Getter node)
   - Drag off its output pin > search `Enum to Int` (or `Byte to Integer`)
   - Connect the integer output to the **Value** pin of Set Graph Parameter Local #5

5. **Trigger generation** — after the last Set Graph Parameter Local:
   - Drag off the exec pin > search `Generate`
   - Select `Generate` (from PCG Component)
   - Connect the **Target** pin to the PCG Component reference
   - In the node details, check **bForce** to `True`

6. **Compile and Save** the Blueprint.

### 0.5 Place in Level and Verify

1. Drag `BP_PCG_Hab` from Content Browser into your level
2. Select it > check the Details panel
3. **Verify:** All 7 parameters appear with sliders/dropdowns
4. Nothing spawns yet (graph nodes not built) — that's expected

**Checkpoint:** Parameters visible in Details panel. Move to Step 1.

---

## Step 1: Floor Grid (GET THIS WORKING FIRST)

> **IMPORTANT:** Complete this step and verify tiles appear before moving to anything else. This is the foundation — if this doesn't work, nothing else will.

Open `PCG_Hab_Main` in the PCG Graph editor (double-click it in Content Browser).

### 1.1 Create Points Grid

1. Right-click in the graph > search **Create Points Grid** > add it
2. Connect the **Input** node's `In` output to this node's `Execution Dependency` input (drag a wire)
3. **Select the node** and look at the **Details panel** on the right. Set these properties:

| Property | Value | Where to Find It |
|----------|-------|-------------------|
| Cell Size | `(X=500, Y=500, Z=500)` | Under "Settings" section, expand CellSize |
| Grid Extents | `(X=1250, Y=500, Z=0)` | Under "Settings" section, expand GridExtents |
| Point Position | `Cell Center` | Dropdown under "Settings" |

> **What this does:** Creates a grid of points centered on the actor's origin.
> With CellSize=500 and GridExtents=(1250,500,0):
> - X: 5 columns (points at X = -1000, -500, 0, 500, 1000)
> - Y: 2 rows (points at Y = -250, 250)
> - Z: 1 layer (Z = 0)
> - Total: 10 points

### 1.2 Shift Points to Positive Quadrant

1. Right-click > search **Transform Points** > add it
2. Connect from **Create Points Grid** `Out` pin to **Transform Points** `In` pin
3. **Select the node.** In the Details panel, set:

| Property | Value | Where to Find It |
|----------|-------|-------------------|
| Offset Min | `(X=1250, Y=500, Z=0)` | Under "Settings" > Offset section |
| Offset Max | `(X=1250, Y=500, Z=0)` | Same as Min (deterministic, not random) |

> **What this does:** Shifts all points so the grid starts at (250, 250, 0) instead of being centered on origin.
> After shift: X = 250, 750, 1250, 1750, 2250 and Y = 250, 750
> These are the CENTER positions of each 500cm floor tile.

### 1.3 Spawn Floor Tiles

1. Right-click > search **Static Mesh Spawner** > add it
2. Connect from **Transform Points** `Out` to **Static Mesh Spawner** `In`
3. **Select the Static Mesh Spawner node.** In the Details panel:
   - Find **Mesh Entries** (under Settings, may need to expand the mesh selector section)
   - Click the **+** button to add a mesh entry
   - In the new entry, find the **Static Mesh** property
   - Click the dropdown/picker > search for `SM_Modular_Merged`
   - Select: `SM_Modular_Merged` from `ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Floor/`
   - Full path: `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Floor/SM_Modular_Merged`

4. **Save** the PCG graph (Ctrl+S).

### 1.4 TEST — Floor Grid Must Appear

1. Go back to your level with `BP_PCG_Hab` placed
2. Select the actor
3. In the Details panel, find the **PCG Component** section
4. Click **Generate** button (or the generation should trigger automatically if the Construction Script runs)
5. **You should see 10 floor tiles** in a 5x2 grid pattern

**If you see tiles: SUCCESS!** The basic PCG pipeline works. Continue below.

**If you DON'T see tiles, troubleshoot:**
- Open the PCG Graph editor, click **Force Regen** in the toolbar while the actor is selected
- Check that the Static Mesh Spawner has a valid mesh entry (not empty)
- Check that the connection wires are blue (connected), not gray (broken)
- Check that Cell Size is 500 (not 100, which is the default)
- Check that Grid Extents has non-zero X and Y values
- Try placing a fresh `BP_PCG_Hab` actor

**Checkpoint:** 10 floor tiles visible in viewport.

---

### 1.5 Compute Grid Index X

Now we'll add attribute math to calculate grid indices for parametric sizing.

1. **Disconnect** the wire between Transform Points and Static Mesh Spawner (right-click the wire > Break Link, or drag off and release in empty space)
2. Right-click > search **Metadata Maths** > add it (this is the "Attribute Operation" node in UE5.7)
3. Connect **Transform Points** `Out` > **Metadata Maths** `InA`
4. **Select the Metadata Maths node.** In the Details panel, set:

| Property | Value | How to Set It |
|----------|-------|---------------|
| Operation | `Divide` | Dropdown at the top of Settings |
| Input Source 1 > Attribute Name | `$Position` | Type `$Position` in the text field |
| Input Source 1 > Extra Names | `X` | Click **+** next to Extra Names array, type `X` |
| Input Source 2 > Attribute Name | (leave default) | — |
| Input Source 2 > Use as constant | `True` | Check the "Use Constant" checkbox |
| Input Source 2 > Constant value | `500.0` | Type `500` in the constant field |
| Output Target > Attribute Name | `GridIndexX` | Type `GridIndexX` |

> **What this does:** For each point, computes GridIndexX = Position.X / 500.
> After the offset, positions are (250, 750, 1250, 1750, 2250), so indices are (0.5, 1.5, 2.5, 3.5, 4.5).

> **IMPORTANT — Finding these properties:** When you select the Metadata Maths node, the Details panel shows sections like "Input Source 1", "Input Source 2", and "Output Target". Each of these is expandable. Look for **Attribute Name**, **Extra Names**, and any constant/value fields.

> **If you can't find "Extra Names":** It may be labeled differently. Look for a way to select a component (X, Y, or Z) of the $Position attribute. Some versions show a dropdown next to the attribute name. If you truly can't find it, set Attribute Name to `$Position.X` as a single string — some UE5 versions accept dot notation.

### 1.6 Compute Grid Index Y

1. Right-click > search **Metadata Maths** > add another one
2. Connect previous **Metadata Maths** `Out` > new **Metadata Maths** `InA`
3. Set properties (same as 1.5 but for Y):

| Property | Value |
|----------|-------|
| Operation | `Divide` |
| Input Source 1 > Attribute Name | `$Position` |
| Input Source 1 > Extra Names | `Y` |
| Input Source 2 > Use Constant | `True`, value = `500.0` |
| Output Target > Attribute Name | `GridIndexY` |

### 1.7 Filter to HabLength (X axis)

We need to get the HabLength parameter value onto each point, then filter.

1. Right-click > search **User Parameter Get** (or **Get Graph Parameter**) > add it
   - In its Details panel, set **Parameter Name** to `HabLength`
   - The node should now show an output pin labeled `HabLength`

2. Right-click > search **Create Attribute Set** > add it
   - Connect the previous **Metadata Maths** (GridIndexY) `Out` > **Create Attribute Set** `In` (or execution dependency)
   - In Details panel:
     - Expand **Output Target** > set **Attribute Name** to `MaxIndexX`
     - Find the value source — set it to use the **HabLength** graph parameter
     - OR: Set it as a **Double** constant temporarily set to `3` (for testing)

   > **Alternative approach if Create Attribute Set is confusing:**
   > Instead of Create Attribute Set, you can connect the **User Parameter Get** node's output to a **Metadata Maths** node that does: `GridIndexX < HabLength`. But the simplest UE5.7 approach is using the Filter node's built-in threshold.

3. Right-click > search **Filter By Attribute** > add it
   - Connect from Create Attribute Set (or from the GridIndexY Metadata Maths if skipping)
   - In the Details panel, **IMPORTANT — set these carefully:**

| Property | Value | Notes |
|----------|-------|-------|
| **Filter Mode** | `By Value` | This is a dropdown — change from "By Existence" to "By Value"! |
| **Target Attribute** | `GridIndexX` | The attribute to test |
| **Filter Operator** | `Less Than` or `Lesser` | Dropdown |
| **Threshold > Use Constant Threshold** | `True` | Check this box |
| **Threshold > Constant value** | `3` | Use 3 for testing (= HabLength default) |

   > **CRITICAL:** The filter MUST be in **"By Value"** mode, NOT "By Existence". If left on existence mode, it filters everything out and you get zero output. This was the bug that caused the MCP-built graph to produce nothing.

   - Use the **Inside Filter** output pin (points where GridIndexX < 3)

### 1.8 Filter to HabWidth (Y axis)

1. Add another **Filter By Attribute**
2. Connect from the previous filter's **Inside Filter** output
3. Set properties:

| Property | Value |
|----------|-------|
| Filter Mode | `By Value` |
| Target Attribute | `GridIndexY` |
| Filter Operator | `Less Than` |
| Threshold > Use Constant Threshold | `True` |
| Threshold > Constant value | `2` (= HabWidth default) |

4. Use the **Inside Filter** output

> After both filters: only points where GridIndexX < 3 AND GridIndexY < 2 remain.
> From the original 10 points, this keeps 6 points (3 columns x 2 rows).

### 1.9 Floor Spawn Branch

1. From the second filter's **Inside Filter** output, connect to a **Transform Points** node:
   - **Offset Min/Max**: `(X=0, Y=0, Z=0)` (no additional offset needed — points are already at tile centers)

   > Actually, you may not need this Transform Points at all. Try connecting the filter output directly to the Static Mesh Spawner first.

2. Connect to the **Static Mesh Spawner** (the one from step 1.3, or add a new one)
   - Mesh: `SM_Modular_Merged` (floor)

3. **Save** the PCG graph.

### 1.10 TEST — Parametric Floor

1. Go to the level, select `BP_PCG_Hab`
2. You should see **6 floor tiles** (3 wide x 2 deep)
3. In the Details panel, change `HabLength`:
   - Set to 1 > should show 2 tiles (1x2)
   - Set to 6 > should show 12 tiles (6x2)
4. Change `HabWidth`:
   - Set to 1 > should show 3 tiles (3x1)
   - Set to 3 > should show 9 tiles (3x3)
5. Reset to HabLength=3, HabWidth=2

**Checkpoint:** Floor grid appears AND resizes with parameters.

> **Making it truly parametric (instead of hardcoded threshold):**
> Once you confirm the floor works with constant thresholds (3 and 2), you can replace the constant thresholds with attribute-based thresholds:
> 1. Add a **User Parameter Get** node for `HabLength`
> 2. Add a **Create Attribute Set** node to write it as an attribute `MaxIndexX` onto the point data
> 3. On the Filter node, uncheck "Use Constant Threshold" and set Threshold Attribute to `MaxIndexX`
> Repeat for HabWidth/MaxIndexY.
> But get it working with constants first!

---

## Step 2: Wall Perimeter Detection

From the **Inside Filter output** of the HabWidth filter (the "clean grid points"), you need to branch off to wall detection. This runs in **parallel** to the floor branch — PCG supports multiple wires from one output pin.

### 2.1 Compute Max Grid Indices

1. From the clean grid points output, drag a **new wire** (you can connect multiple wires from one output)
2. Add a **Create Attribute Set** node:
   - **Output Attribute Name**: `MaxGridX`
   - **Value**: Graph Parameter `HabLength` (or constant `3` for testing)

3. Add a **Metadata Maths** node after it:
   - **Operation**: Subtract
   - **Input Source 1**: Attribute > `MaxGridX`
   - **Input Source 2**: Constant > `1`
   - **Output Target**: Attribute > `MaxGridX`

4. Add another **Create Attribute Set** node:
   - **Output Attribute Name**: `MaxGridY`
   - **Value**: Graph Parameter `HabWidth` (or constant `2` for testing)

5. Add a **Metadata Maths** node:
   - **Operation**: Subtract
   - **Input Source 1**: Attribute > `MaxGridY`
   - **Input Source 2**: Constant > `1`
   - **Output Target**: Attribute > `MaxGridY`

> Now every point has `MaxGridX` (e.g., 2 for HabLength=3) and `MaxGridY` (e.g., 1 for HabWidth=2).

### 2.2 Four Edge Filters

From the MaxGridY output, create **4 parallel branches** (drag 4 wires from the same output):

#### South Edge (GridIndexY == 0)

1. Add **Filter By Attribute**:
   - **Filter Mode**: `By Value`
   - **Target Attribute**: `GridIndexY`
   - **Filter Operator**: `Equal`
   - **Threshold**: Constant > `0`
   - Use **Inside Filter** output > these are south edge points

#### North Edge (GridIndexY == MaxGridY)

1. Add **Filter By Attribute**:
   - **Filter Mode**: `By Value`
   - **Target Attribute**: `GridIndexY`
   - **Filter Operator**: `Equal`
   - **Threshold**: Attribute > `MaxGridY` (uncheck "Use Constant Threshold")
   - Use **Inside Filter** output > these are north edge points

#### West Edge (GridIndexX == 0)

1. Add **Filter By Attribute**:
   - **Filter Mode**: `By Value`
   - **Target Attribute**: `GridIndexX`
   - **Filter Operator**: `Equal`
   - **Threshold**: Constant > `0`
   - Use **Inside Filter** output > these are west edge points

#### East Edge (GridIndexX == MaxGridX)

1. Add **Filter By Attribute**:
   - **Filter Mode**: `By Value`
   - **Target Attribute**: `GridIndexX`
   - **Filter Operator**: `Equal`
   - **Threshold**: Attribute > `MaxGridX`
   - Use **Inside Filter** output > these are east edge points

**Checkpoint:** No visual change yet — this is pure data setup.

---

## Step 3: Wall Placement

For each edge branch, apply the correct transform offset and rotation for ModularSciFi wall pieces.

### Wall Offset Reference (relative to floor tile center)

| Edge | Offset | Rotation (Yaw) | Why |
|------|--------|----------------|-----|
| South | (X=0, Y=0, Z=0) | Yaw=-90 | Wall sits on south edge of tile |
| North | (X=0, Y=500, Z=0) | Yaw=90 | Wall sits on north edge |
| West | (X=0, Y=0, Z=0) | Yaw=180 | Wall sits on west edge |
| East | (X=500, Y=0, Z=0) | Yaw=0 | Wall sits on east edge |

> These offsets assume the ModularSciFi wall pivot is at one end. You may need to adjust empirically — place one wall manually next to a floor tile to verify alignment before committing.

### 3.1-3.4 Per-Edge Transform

For each edge filter output, add a **Transform Points** node:

**South:** Offset=(0,0,0), Rotation=(0,-90,0)
**North:** Offset=(0,500,0), Rotation=(0,90,0)
**West:** Offset=(0,0,0), Rotation=(0,180,0)
**East:** Offset=(500,0,0), Rotation=(0,0,0)

> Set both OffsetMin and OffsetMax to the same value (deterministic).
> Set both RotationMin and RotationMax to the same value.

### 3.5 Union All Walls

1. Right-click > search **Union** > add it
2. Connect all 4 wall Transform Points outputs into the Union node
   - Click **+** on the Union node to add input pins (you need 4)

### 3.6 Wall Spawner (for testing)

1. Add a **Static Mesh Spawner** after the Union
2. Mesh: `SM_Modular_Wall_1_Merged`
   - Path: `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Wall_1/SM_Modular_Wall_1_Merged`
3. **Save** the PCG graph.

### 3.7 TEST — Walls

1. Select `BP_PCG_Hab` in the level
2. You should see walls on all 4 edges of the floor grid
3. Walls should face **outward** (textured side away from interior)
4. Change HabLength/HabWidth — walls should update

**Checkpoint:** Walls on all 4 edges, facing outward, resize with parameters.

> **If walls face inward:** Flip the Yaw by 180 degrees on that edge's Transform Points.
> **If walls are offset wrong:** Place one wall mesh manually in the level next to a floor tile. Note the position/rotation needed, then adjust the Transform Points to match.

---

## Step 4-8: Doors, Windows, Corners, Roof, Details

These steps remain the same as the original guide. The key principles:

### Step 4: Door Replacement
- Filter wall points by `WallDirection == DoorSideIndex`
- Filter door-side walls by `WallSegmentIndex < DoorCount`
- Tag with `IsDoor` attribute (0 or 1)

### Step 5: Window Randomization
- Use **Density Noise** for random values per point
- Filter by `$Density < WindowDensity` for window placement
- Three spawners: Door mesh, Window mesh (weighted random), Solid wall mesh

### Step 6: Corners
- Filter for the 4 corner points (GridIndexX==0 AND GridIndexY==0, etc.)
- Apply per-corner transform offsets and rotations
- Spawn corner mesh at each

### Step 7: Roof
- Branch from clean grid, set `$Density` from `HasRoof` parameter
- Density Filter (0.5-1.0) acts as on/off switch
- Transform Z+500, spawn roof mesh

### Step 8: Detail Scatter (stretch goal)
- Branch from wall points, Density Noise > Filter 30%
- Transform Z+250 (mid-wall height)
- Weighted random spawner with vent, bracket, lamp, pump meshes

---

## Mesh Reference Table (All Paths Verified)

| Role | Full Content Path |
|------|-------------------|
| Floor | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Floor/SM_Modular_Merged` |
| Wall (solid) | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Wall_1/SM_Modular_Wall_1_Merged` |
| Wall (window 1) | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_WallWindow_1/SM_WallWindow_1_Merged` |
| Wall (window 2) | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_WallWindow_2/SM_Modular_WallWindow_2_Merged` |
| Wall (window 3) | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_WallWindow_3/SM_Modular_WallWindow_3_Merged` |
| Door | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Door_Var1/SM_Modular_DoorModule_Merged` |
| Corner | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Corner/SM_Modular_Corner_Merged` |
| Roof | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Ceiling/SM_Modular_RoofModule_Merged` |

---

## Troubleshooting Checklist

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| No output at all | Filter in "By Existence" mode | Change Filter Mode to "By Value" |
| No output at all | Grid Extents all zero | Set GridExtents to (1250, 500, 0) |
| No output at all | CellSize at default 100 | Set CellSize to (500, 500, 500) |
| No output at all | Static Mesh Spawner has no mesh | Add a mesh entry and pick a mesh |
| No output at all | Wires disconnected | Check all connections are blue, not gray |
| No output at all | Generation not triggering | Select actor > PCG Component > click Generate |
| Floor at wrong position | Offset values wrong | Check TransformPoints OffsetMin/Max match the table |
| Walls facing wrong way | Yaw rotation off by 180 | Add/subtract 180 from that edge's yaw |
| Parameters don't update | Construction Script not calling Generate | Verify CS chain ends with Generate(Force=true) |
| Parameters don't update | Parameter names don't match | CS strings must EXACTLY match PCG graph parameter names |

---

## Grid Math Reference

With CellSize=(500,500,500) and GridExtents=(1250,500,0), PointPosition=CellCenter:

- PointCountX = floor(2 * 1250 / 500) = 5
- PointCountY = floor(2 * 500 / 500) = 2
- PointCountZ = max(1, floor(2 * 0 / 500)) = 1
- Total: 5 x 2 x 1 = **10 points**

Point positions (CellCenter adds 0.5 cell offset):
- X: -1000, -500, 0, 500, 1000
- Y: -250, 250
- Z: 0

After TransformPoints offset (1250, 500, 0):
- X: 250, 750, 1250, 1750, 2250
- Y: 250, 750

GridIndexX = X / 500: 0.5, 1.5, 2.5, 3.5, 4.5
GridIndexY = Y / 500: 0.5, 1.5

Filter GridIndexX < 3 (HabLength): keeps 0.5, 1.5, 2.5 (3 columns)
Filter GridIndexY < 2 (HabWidth): keeps 0.5, 1.5 (2 rows)
Result: **6 floor tiles** for HabLength=3, HabWidth=2
