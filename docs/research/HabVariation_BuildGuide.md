# Hab Variation System — Build Guide (UE5.7)

> **Purpose:** Create a reusable DataAsset + Spawner system that reproduces hand-built hab layouts from data. Complements the existing PCG Hab Generator with pre-designed variations.

---

## Overview

The system has 5 assets:

| Asset | Type | Location |
|-------|------|----------|
| `E_HabPieceType` | Enumeration | `/Content/Custom/PCG/HabGenerator/` |
| `S_HabPiece` | Structure | `/Content/Custom/PCG/HabGenerator/` |
| `DA_HabVariation` | Data Asset Class | `/Content/Custom/PCG/HabGenerator/` |
| `DA_HabVar_Compact45` | Data Asset Instance | `/Content/Custom/PCG/HabGenerator/Variations/` |
| `BP_HabVariationSpawner` | Blueprint Actor | `/Content/Custom/PCG/HabGenerator/` |

---

## Piece Manifest — L5 Skylight Compact Hab

**Origin:** Center floor tile (StaticMeshActor_36)
**World Origin:** (-295878, -129518, -25707)
**Base Rotation:** Yaw = -45 (factored out — template is rotation-agnostic)
**Grid:** 500cm cells on the ModularSciFi kit

### Floor Layout (local grid)

```
          [North]     Y=+500
          [Center]    Y=0
[SW]      [South]     Y=-500   (SW at X=-500)
          [FarSouth]  Y=-1000
```

### Complete Piece Table (26 pieces)

All positions are in **local space** relative to the hab origin. When the spawner is at rotation 0, pieces appear at these offsets. Rotating the spawner rotates the entire hab.

#### Floors (5)

| # | Mesh | RelX | RelY | RelZ | RelYaw | Label |
|---|------|------|------|------|--------|-------|
| 1 | SM_Modular_Merged | 0 | 0 | 0 | 0 | Center |
| 2 | SM_Modular_Merged | 0 | -500 | 0 | 0 | South |
| 3 | SM_Modular_Merged | 0 | -1000 | 0 | 0 | Far South |
| 4 | SM_Modular_Merged | 0 | 500 | 0 | 0 | North |
| 5 | SM_Modular_Merged | -500 | -500 | 0 | 0 | Southwest |

#### Walls (2)

| # | Mesh | RelX | RelY | RelZ | RelYaw | Label |
|---|------|------|------|------|--------|-------|
| 6 | SM_Modular_Wall_1_Merged | 500 | 1000 | 0 | 90 | NE wall |
| 7 | SM_Modular_Wall_1_Merged | 0 | 500 | 0 | -90 | NW wall |

#### Windows (3)

| # | Mesh | RelX | RelY | RelZ | RelYaw | Label |
|---|------|------|------|------|--------|-------|
| 8 | SM_Modular_WallWindow_2_Merged | 500 | 0 | 0 | 90 | East center |
| 9 | SM_Modular_WallWindow_3_Merged | 500 | -500 | 0 | 90 | East south |
| 10 | SM_WallWindow_1_Merged | 500 | 500 | 0 | 90 | East north |

#### Doors (3)

| # | Mesh | RelX | RelY | RelZ | RelYaw | Label |
|---|------|------|------|------|--------|-------|
| 11 | SM_Modular_DoorModule_Merged | -500 | -500 | 0 | -90 | SW door |
| 12 | SM_Modular_DoorModule_Merged | 500 | -1000 | 0 | 0 | SE door |
| 13 | SM_Modular_DoorModule_Merged | 0 | 1000 | 0 | 180 | North door |

#### Corner Trims (6)

| # | Mesh | RelX | RelY | RelZ | RelYaw | Label |
|---|------|------|------|------|--------|-------|
| 14 | SM_Modular_Corner_Merged | 0 | -1000 | 0 | -90 | SE far corner |
| 15 | SM_Modular_Corner_Merged | 500 | -1000 | 0 | 0 | E far corner |
| 16 | SM_Modular_Corner_Merged | 500 | 1000 | 0 | 90 | NE corner |
| 17 | SM_Modular_Corner_Merged | 0 | 1000 | 0 | 180 | N corner |
| 18 | SM_Modular_Corner_Merged | -500 | -500 | 0 | -90 | SW corner |
| 19 | SM_Modular_Corner_Merged | -500 | 0 | 0 | 180 | W corner |

#### Diagonal Corners (2)

| # | Mesh | RelX | RelY | RelZ | RelYaw | Label |
|---|------|------|------|------|--------|-------|
| 20 | SM_Modular_WallCorner_2_Merged | 0 | -500 | 0 | 180 | Inner diagonal south |
| 21 | SM_Modular_WallCorner_2_Merged | 0 | 0 | 0 | 90 | Inner diagonal center |

#### Roof Panels (3)

| # | Mesh | RelX | RelY | RelZ | RelYaw | Label |
|---|------|------|------|------|--------|-------|
| 22 | SM_Modular_RoofModule_Merged | 0 | 0 | 0 | 0 | Center roof |
| 23 | SM_Modular_RoofModule_Merged | 0 | 500 | 0 | 0 | North roof |
| 24 | SM_Modular_RoofModule_Merged | 0 | -1000 | 0 | 0 | Far south roof |

#### Skylight Panels (2)

| # | Mesh | RelX | RelY | RelZ | RelYaw | Label |
|---|------|------|------|------|--------|-------|
| 25 | SM_Modular_Ceiling_Window2_Merged | 0 | 0 | 0 | -90 | Center skylight |
| 26 | SM_Modular_Ceiling_Window2_Merged | 0 | -500 | 0 | 90 | South skylight |

### Mesh Path Reference

| Short Name | Full Content Path |
|------------|-------------------|
| SM_Modular_Merged | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Floor/SM_Modular_Merged` |
| SM_Modular_Wall_1_Merged | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Wall_1/SM_Modular_Wall_1_Merged` |
| SM_Modular_WallWindow_2_Merged | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_WallWindow_2/SM_Modular_WallWindow_2_Merged` |
| SM_Modular_WallWindow_3_Merged | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_WallWindow_3/SM_Modular_WallWindow_3_Merged` |
| SM_WallWindow_1_Merged | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_WallWindow_1/SM_WallWindow_1_Merged` |
| SM_Modular_DoorModule_Merged | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Door_Var1/SM_Modular_DoorModule_Merged` |
| SM_Modular_Corner_Merged | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Corner/SM_Modular_Corner_Merged` |
| SM_Modular_WallCorner_2_Merged | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_WallCorner_2/SM_Modular_WallCorner_2_Merged` |
| SM_Modular_RoofModule_Merged | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Ceiling/SM_Modular_RoofModule_Merged` |
| SM_Modular_Ceiling_Window2_Merged | `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/Modular_Ceiling_Window2/SM_Modular_Ceiling_Window2_Merged` |

---

## Step 1: Create Enum `E_HabPieceType`

1. In Content Browser, navigate to `/Content/Custom/PCG/HabGenerator/`
2. Right-click > **Blueprints > Enumeration**
3. Name it `E_HabPieceType`
4. Open it and add 6 enumerators (click **Add Enumerator** for each):

| Index | Display Name |
|-------|-------------|
| 0 | Floor |
| 1 | Wall |
| 2 | Window |
| 3 | Door |
| 4 | Corner |
| 5 | Roof |

5. Save and close.

**Checkpoint:** Enum appears in Content Browser with 6 values.

---

## Step 2: Create Structure `S_HabPiece`

1. In `/Content/Custom/PCG/HabGenerator/`, right-click > **Blueprints > Structure**
2. Name it `S_HabPiece`
3. Open it and add 4 variables (click **Add Variable** for each):

| Variable | Type | Description |
|----------|------|-------------|
| `Mesh` | **Static Mesh** (Object Reference) | The mesh asset to spawn |
| `RelativeLocation` | **Vector** | Offset from hab origin (cm) |
| `RelativeRotation` | **Rotator** | Rotation relative to hab facing |
| `PieceType` | **E_HabPieceType** | Floor / Wall / Window / Door / Corner / Roof |

> **Important:** For the Mesh field, use type `Static Mesh` (under Object Reference), NOT Soft Object Reference. This keeps it simple for Phase 1. You can convert to Soft References later if memory becomes a concern.

4. Save and close.

**Checkpoint:** Structure has 4 fields. Verify by hovering over it in Content Browser.

---

## Step 3: Create Data Asset Class `DA_HabVariation`

1. In `/Content/Custom/PCG/HabGenerator/`, right-click > **Blueprints > Blueprint Class**
2. In the parent class picker, expand **All Classes** and search for `PrimaryDataAsset`
3. Select `PrimaryDataAsset` as the parent class
4. Name it `DA_HabVariation`
5. Open it and add 3 variables:

| Variable | Type | Default | Flags |
|----------|------|---------|-------|
| `VariationName` | **String** | "" | Instance Editable, BlueprintReadWrite |
| `Description` | **String** | "" | Instance Editable, BlueprintReadWrite |
| `Pieces` | **Array of S_HabPiece** | empty | Instance Editable, BlueprintReadWrite |

> To make a variable Instance Editable: click the closed-eye icon next to the variable name so it becomes an open eye.

6. Compile and Save.

**Checkpoint:** `DA_HabVariation` has 3 variables. The `Pieces` array accepts `S_HabPiece` elements.

---

## Step 4: Create Data Asset Instance `DA_HabVar_Compact45`

1. In Content Browser, create a subfolder: `/Content/Custom/PCG/HabGenerator/Variations/`
2. In that folder, right-click > **Miscellaneous > Data Asset**
3. In the class picker, select `DA_HabVariation`
4. Name it `DA_HabVar_Compact45`
5. Open it and set:
   - **VariationName:** `Compact45`
   - **Description:** `L-shaped 5-cell compact hab with skylight corridor and 3 doors`
6. In the **Pieces** array, click the **+** button 26 times to create 26 entries
7. Fill each entry using the piece table below

### Data Entry — All 26 Pieces

For each row, expand the array element and set Mesh (use Content Browser picker), RelativeLocation (X,Y,Z), RelativeRotation (Pitch=0, Yaw=value, Roll=0), and PieceType.

**Tip:** After setting the first floor piece, you can duplicate entries and just change the differing fields.

| # | Mesh (browse to) | Rel Location (X, Y, Z) | Rel Rotation (P, Y, R) | PieceType |
|---|---|---|---|---|
| 1 | `SM_Modular_Merged` | (0, 0, 0) | (0, 0, 0) | Floor |
| 2 | `SM_Modular_Merged` | (0, -500, 0) | (0, 0, 0) | Floor |
| 3 | `SM_Modular_Merged` | (0, -1000, 0) | (0, 0, 0) | Floor |
| 4 | `SM_Modular_Merged` | (0, 500, 0) | (0, 0, 0) | Floor |
| 5 | `SM_Modular_Merged` | (-500, -500, 0) | (0, 0, 0) | Floor |
| 6 | `SM_Modular_Wall_1_Merged` | (500, 1000, 0) | (0, 90, 0) | Wall |
| 7 | `SM_Modular_Wall_1_Merged` | (0, 500, 0) | (0, -90, 0) | Wall |
| 8 | `SM_Modular_WallWindow_2_Merged` | (500, 0, 0) | (0, 90, 0) | Window |
| 9 | `SM_Modular_WallWindow_3_Merged` | (500, -500, 0) | (0, 90, 0) | Window |
| 10 | `SM_WallWindow_1_Merged` | (500, 500, 0) | (0, 90, 0) | Window |
| 11 | `SM_Modular_DoorModule_Merged` | (-500, -500, 0) | (0, -90, 0) | Door |
| 12 | `SM_Modular_DoorModule_Merged` | (500, -1000, 0) | (0, 0, 0) | Door |
| 13 | `SM_Modular_DoorModule_Merged` | (0, 1000, 0) | (0, 180, 0) | Door |
| 14 | `SM_Modular_Corner_Merged` | (0, -1000, 0) | (0, -90, 0) | Corner |
| 15 | `SM_Modular_Corner_Merged` | (500, -1000, 0) | (0, 0, 0) | Corner |
| 16 | `SM_Modular_Corner_Merged` | (500, 1000, 0) | (0, 90, 0) | Corner |
| 17 | `SM_Modular_Corner_Merged` | (0, 1000, 0) | (0, 180, 0) | Corner |
| 18 | `SM_Modular_Corner_Merged` | (-500, -500, 0) | (0, -90, 0) | Corner |
| 19 | `SM_Modular_Corner_Merged` | (-500, 0, 0) | (0, 180, 0) | Corner |
| 20 | `SM_Modular_WallCorner_2_Merged` | (0, -500, 0) | (0, 180, 0) | Corner |
| 21 | `SM_Modular_WallCorner_2_Merged` | (0, 0, 0) | (0, 90, 0) | Corner |
| 22 | `SM_Modular_RoofModule_Merged` | (0, 0, 0) | (0, 0, 0) | Roof |
| 23 | `SM_Modular_RoofModule_Merged` | (0, 500, 0) | (0, 0, 0) | Roof |
| 24 | `SM_Modular_RoofModule_Merged` | (0, -1000, 0) | (0, 0, 0) | Roof |
| 25 | `SM_Modular_Ceiling_Window2_Merged` | (0, 0, 0) | (0, -90, 0) | Roof |
| 26 | `SM_Modular_Ceiling_Window2_Merged` | (0, -500, 0) | (0, 90, 0) | Roof |

8. Save.

**Checkpoint:** DataAsset has 26 entries. Scroll through to verify meshes are assigned (not None).

---

## Step 5: Create Spawner Blueprint `BP_HabVariationSpawner`

### 5.1 Create the Blueprint

1. In `/Content/Custom/PCG/HabGenerator/`, right-click > **Blueprints > Blueprint Class**
2. Select **Actor** as parent class
3. Name it `BP_HabVariationSpawner`
4. Open it

### 5.2 Add Variables

| Variable | Type | Default | Flags |
|----------|------|---------|-------|
| `HabVariation` | **DA_HabVariation** (Object Reference) | None | Instance Editable, Expose on Spawn |

> To set **Expose on Spawn**: select the variable, check the box in the Details panel on the right.

### 5.3 Add a Default Scene Root

1. In the Components panel, click **Add** > search **Default Scene Root**
2. This is already there by default. Just verify the root component exists.

### 5.4 Build the Construction Script

Open the **Construction Script** tab. Build this graph:

```
[Construction Script]
    → [Is Valid] (test HabVariation)
        → [Branch]
            True →
                [Get Pieces] (from HabVariation)
                    → [For Each Loop] (Array: Pieces)
                        → [Add Static Mesh Component] (target: Self)
                            ← Return: NewComponent
                        → [Set Static Mesh] (target: NewComponent, mesh: ArrayElement.Mesh)
                        → [Set Relative Location] (target: NewComponent, location: ArrayElement.RelativeLocation)
                        → [Set Relative Rotation] (target: NewComponent, rotation: ArrayElement.RelativeRotation)
```

#### Node-by-Node Instructions:

1. **From the Construction Script node**, drag off the exec pin

2. **IsValid check:**
   - Right-click > search `Is Valid` (Utilities > Is Valid)
   - Connect `HabVariation` variable (drag from My Blueprint panel) to the input
   - This outputs an exec pin for valid/invalid

3. **Branch (implicit from IsValid):**
   - `Is Valid` already branches — drag from the **Is Valid** exec pin

4. **Get Pieces array:**
   - Drag off the `HabVariation` variable reference
   - Type `.` or search `Pieces` to get the Pieces array
   - This gives you an Array of S_HabPiece

5. **For Each Loop:**
   - Right-click > search `For Each Loop`
   - Connect the Pieces array to the **Array** input
   - The loop provides `Array Element` (one S_HabPiece) and `Array Index`

6. **Break S_HabPiece:**
   - Drag off `Array Element` > search `Break S_HabPiece`
   - This gives you: Mesh, RelativeLocation, RelativeRotation, PieceType

7. **Add Static Mesh Component:**
   - From the **Loop Body** exec pin, add node `Add Static Mesh Component`
   - **Target:** Self (default)
   - **Static Mesh:** connect from `Break S_HabPiece → Mesh`
   - **Relative Transform:** Make Transform node:
     - **Location:** connect from `Break S_HabPiece → RelativeLocation`
     - **Rotation:** connect from `Break S_HabPiece → RelativeRotation`
     - **Scale:** leave as (1,1,1)
   - **Manual Attachment:** leave unchecked
   - The component auto-attaches to the root

8. **Compile and Save**

#### Alternative (Simpler) Construction Script:

If the Make Transform approach feels complex, you can do it in 3 separate steps after `Add Static Mesh Component`:

```
Loop Body →
    [Add Static Mesh Component] (Static Mesh = piece.Mesh)
        Return: NewComp
    → [Set Relative Location and Rotation]
        Target: NewComp
        New Location: piece.RelativeLocation
        New Rotation: piece.RelativeRotation
```

Search for `Set Relative Location and Rotation` — it's a single node that takes both.

### 5.5 Compile and Save

**Checkpoint:** Blueprint compiles with no errors. The Construction Script has a loop that adds mesh components.

---

## Step 6: Test & Verify

1. **Place the spawner:**
   - Drag `BP_HabVariationSpawner` from Content Browser into the level
   - Place it somewhere open (away from the original hand-built hab)

2. **Assign the DataAsset:**
   - Select the spawner in the viewport
   - In the Details panel, find `Hab Variation`
   - Click the dropdown and select `DA_HabVar_Compact45`
   - The hab should appear immediately (Construction Script runs on property change)

3. **Verify the layout:**
   - Compare with the original hand-built hab near (-295878, -129518)
   - All 26 pieces should be present: 5 floors, 2 walls, 3 windows, 3 doors, 8 corners, 5 roof/skylights

4. **Test rotation:**
   - Rotate the spawner actor — all pieces should rotate together as one unit
   - Try rotating to yaw=-45 — it should match the original hand-built hab's orientation

5. **Test movement:**
   - Move the spawner — all pieces move together

6. **Test duplication:**
   - Select the spawner > Ctrl+D to duplicate
   - The copy should be identical

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Hab doesn't appear | Check that `HabVariation` is set (not None) in Details panel |
| Missing pieces | Verify all 26 entries in the DataAsset have valid meshes (not None) |
| Pieces at wrong positions | Verify RelativeLocation values match the table exactly |
| Pieces floating/sunk | All RelZ should be 0 — check the Z component of RelativeLocation |
| Compile error: "Pieces not found" | Make sure `Pieces` variable in `DA_HabVariation` is public (Instance Editable) |
| Mesh picker is empty | Navigate to `/Game/ModularSciFi/StaticMeshes/HardSurfaceEnvironment/` in the picker |

---

## Future Extensions

- **More variations:** Create new `DA_HabVar_*` instances with different layouts. No new Blueprints needed.
- **PCG integration:** `BP_PCG_Hab` can accept a `DA_HabVariation` parameter as an alternative to parametric generation.
- **Settlement layout:** A master Blueprint could spawn multiple `BP_HabVariationSpawner` instances in a pattern.
- **Material overrides:** Add a material slot to `S_HabPiece` for per-piece material customization.
- **Collision presets:** The Static Mesh Components inherit collision from the meshes. No extra setup needed.

---

## Cross-References

| File | Purpose |
|------|---------|
| `docs/guide/PCG_Hab_BuildGuide.md` | Parametric PCG hab generator (Phase 1) |
| `docs/hab_variations/variation_L5_skylight.json` | Raw JSON data for this variation |
| `CLAUDE.md` | Project architecture and conventions |
