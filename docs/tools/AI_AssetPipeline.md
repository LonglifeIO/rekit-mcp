
AI-DRIVEN ASSET PIPELINE
MCP-Orchestrated Asset Management for UE5 + Blender
Free & Open-Source Edition
Technical Blueprint & Implementation Guide
ThoughtSpace Studio
Halifax, Nova Scotia • February 2026



1. Executive Summary
This document is a buildable technical plan for an AI-driven asset pipeline that lets a solo developer manage, retrieve, convert, combine, retexture, and deploy 3D assets across Unreal Engine 5 and Blender using MCP (Model Context Protocol) servers orchestrated by Claude.
The pipeline solves a core solo-dev problem: you own hundreds of UE Marketplace assets but they sit unused in your Vault because the friction of finding, downloading, importing, and adapting them to your visual style is too high. This system eliminates that friction.
ZERO-COST COMMITMENT
Every tool in this pipeline is free, open-source, or custom-built. No paid API keys, no subscriptions,
no per-generation fees. The only cost is your GPU (NVIDIA, 8 GB+ VRAM minimum, 12 GB+ recommended).
All tools run locally on your machine — no cloud dependency, no usage limits, full control.

What This Pipeline Does
1. Catalogs every asset you own in a searchable, tagged SQLite database without downloading them
2. Downloads on demand only the specific asset packs needed for the current task
3. Converts & transfers assets between UE5 and Blender via automated FBX export
4. Kit-bashes multiple meshes into unified objects in Blender (mesh joining, boolean ops)
5. Retextures combined meshes to match your game’s visual style via local AI texture generation
6. Generates PBR maps (albedo, normal, roughness, metallic, height) from AI-painted surfaces
7. Deploys finished assets back into UE5 levels, properly placed and configured
Claude orchestrates the entire workflow through natural language. You describe what you want to build, and the system selects assets, downloads what’s missing, assembles geometry in Blender, paints and textures the result using local AI, and places it in your UE5 level.

2. System Architecture
The pipeline consists of six layers, each handled by a different free tool or MCP server. Claude sits at the top as the orchestration layer, chaining calls across all six.
2.1 Architecture Layers
Layer
Tool
Cost
Role
Orchestration
Claude (Desktop / Code)
Free tier
Receives natural language requests, plans multi-step workflows, chains MCP calls
Catalog & Download
UEVaultManager + Custom MCP
Free (OSS)
Indexes owned assets in SQLite, downloads on demand via Epic API
Scene Placement
Flopperam UE5 MCP
Free (OSS)
Spawns actors, discovers assets in Content folder, sets transforms/materials
Mesh Fabrication
Blender MCP (ahujasid)
Free (OSS)
Imports FBX, joins meshes, boolean operations, UV cleanup, exports combined mesh
AI Texturing
StableGen + ComfyUI (local)
Free (OSS)
Paints style-consistent textures on meshes from multiple viewpoints in Blender
PBR Generation
Material Anything / MatForger
Free (OSS)
Generates full PBR map sets (normal, roughness, metallic, height) from painted albedo

2.2 Data Flow
The following describes the complete data flow for a typical request like “Build me a Europa reactor corridor using my asset library.”

COMPLETE PIPELINE FLOW
1. CATALOG QUERY → Claude searches your SQLite asset database by tags ("Europa", "industrial", "corridor")
2. DOWNLOAD → UEVaultManager downloads matching packs not yet on disk to your UE project Content folder
3. UE EXPORT → UE5 MCP runs Python script to batch-export selected static meshes as FBX to shared folder
4. BLENDER IMPORT → Blender MCP imports the FBX files into a fresh scene
5. KIT-BASH → Blender MCP arranges, scales, and joins meshes into a single unified object
6. AI TEXTURE → StableGen (Blender addon + local ComfyUI) paints your style onto the combined mesh
7. PBR MAPS → Material Anything or CHORD decomposes painted surface into full PBR map set
8. EXPORT → Blender MCP exports the final textured mesh as FBX back to the shared folder
9. UE IMPORT & PLACE → UE5 MCP imports the new asset into Content and spawns it in the level

2.3 Folder Structure
All tools communicate through a shared folder structure on your local machine:
D:/GameDev/
  ├─ AssetPipeline/
  │   ├─ catalog/            ← SQLite DB + manifest files
  │   ├─ exchange/           ← FBX transfer folder (UE ↔ Blender)
  │   ├─ retextured/         ← AI-textured assets ready for import
  │   └─ style-prompts/      ← Saved texture style templates
  ├─ ComfyUI/                ← Local ComfyUI installation (StableGen backend)
  ├─ UE5Projects/
  │   └─ WarpBorn/Content/   ← Active UE5 project
  └─ BlenderWorkspace/       ← Blender working files

3. Component Deep Dives
3.1 Asset Catalog: UEVaultManager
UEVaultManager is an open-source Python CLI that interfaces with Epic’s backend services to list, filter, and download your owned Marketplace assets. It stores metadata in SQLite for fast querying. License: based on Legendary (GPL-compatible). Cost: free.
Installation
pip install UEVaultManager
After installation, authenticate with your Epic account:
UEVaultManager auth
This opens a browser for Epic Games login. Your session token is cached locally.
Building Your Catalog
Run the initial scan to build the asset database. This pulls metadata for every asset you own (descriptions, categories, version compatibility, thumbnails) without downloading the actual files:
UEVaultManager list --output catalog/my_vault.db
This may take several minutes on the first run. Subsequent runs use cached data and are much faster. The resulting SQLite database is your searchable asset library.
Downloading Assets On Demand
When the pipeline identifies an asset pack that’s needed but not yet on disk:
UEVaultManager install <asset-name> --install-folder D:/GameDev/UE5Projects/WarpBorn/Content/
This downloads and extracts the asset pack directly into your UE5 project’s Content folder, including all dependencies.
Adding Custom Tags
Extend the SQLite database with a custom tags table for your universe:
CREATE TABLE custom_tags (asset_id TEXT, tag TEXT, PRIMARY KEY (asset_id, tag));
INSERT INTO custom_tags VALUES ('asset_abc123', 'europa');
INSERT INTO custom_tags VALUES ('asset_abc123', 'industrial');
Over time, as you use assets and tag them, this becomes an increasingly powerful retrieval system. Claude can query it with: SELECT * FROM custom_tags WHERE tag IN ('europa', 'industrial') to find relevant packs.
Known Limitations
Epic has no official public API for Vault access. UEVaultManager reverse-engineers the Epic Games Service API, which means updates from Epic could temporarily break functionality.
Epic recently added captcha systems that can interfere with automated scraping. UEVaultManager v1.10+ includes workarounds, but this remains a fragility point.
Large asset packs (1–5 GB) take time to download. Pre-download your most-used packs and use on-demand downloading only for less common assets.

3.2 Custom MCP Server: Asset Library Bridge
This is the glue component you build yourself. It’s a Python MCP server that exposes your asset catalog as tools Claude can call directly. Cost: free (your own code). License: yours.
Server Tools
Tool Name
Purpose
Example Call
search_assets
Query catalog by tags, name, category, UE version
search_assets(tags=["sci-fi", "corridor"], ue_version="5.5")
get_asset_details
Return full metadata for a specific asset pack
get_asset_details(asset_id="abc123")
check_downloaded
Check if an asset pack is already on disk
check_downloaded(asset_id="abc123")
download_asset
Download a pack to the UE project Content folder
download_asset(asset_id="abc123", target_project="WarpBorn")
list_meshes_in_pack
List individual static meshes within a downloaded pack
list_meshes_in_pack(asset_id="abc123")
add_tags
Add custom tags to an asset for future retrieval
add_tags(asset_id="abc123", tags=["europa", "reactor"])
get_style_prompt
Retrieve saved style prompt templates
get_style_prompt(style="europa_industrial")

Implementation Skeleton
# asset_library_mcp.py
from mcp.server import Server
from mcp.types import Tool, TextContent
import sqlite3, subprocess, json, os

DB_PATH = "D:/GameDev/AssetPipeline/catalog/my_vault.db"
CONTENT_PATH = "D:/GameDev/UE5Projects/WarpBorn/Content/"

server = Server("asset-library")

@server.tool()
async def search_assets(tags: list[str], ue_version: str = None):
    """Search owned assets by custom tags and UE version."""
    conn = sqlite3.connect(DB_PATH)
    # Query joins vault metadata with custom_tags table
    # Returns asset_id, name, description, categories, download_size
    ...

@server.tool()
async def download_asset(asset_id: str, target_project: str):
    """Download asset pack via UEVaultManager CLI."""
    result = subprocess.run([
        "UEVaultManager", "install", asset_id,
        "--install-folder", os.path.join(CONTENT_PATH, "AssetLibrary/")
    ], capture_output=True)
    return TextContent(text=f"Downloaded {asset_id}: {result.stdout}")
MCP Configuration
{
  "asset-library": {
    "command": "python",
    "args": ["D:/GameDev/AssetPipeline/asset_library_mcp.py"]
  }
}

3.3 UE5 MCP: Scene Placement & Asset Export
The Flopperam UE5 MCP handles two critical roles: spawning assets in your level AND exporting meshes to FBX for the Blender handoff. License: open-source. Cost: free.
Asset Discovery
The UE MCP can scan your project’s Content folder to discover available assets. Once UEVaultManager has downloaded a pack, the UE MCP can immediately find and use those assets:
# Claude prompt: "What meshes are available in my industrial corridor pack?"
# UE MCP calls: discover_assets(path="/Game/AssetLibrary/IndustrialCorridors/", type="StaticMesh")
Batch FBX Export (UE to Blender)
The UE MCP can execute Python scripts inside the UE5 editor to export selected static meshes:
import unreal

meshes_to_export = [
    "/Game/AssetLibrary/Pipes/SM_Pipe_Cluster_01",
    "/Game/AssetLibrary/Walls/SM_Wall_Metal_Panel",
    "/Game/AssetLibrary/Props/SM_Console_Monitor",
]

export_task = unreal.AssetExportTask()
export_task.automated = True
export_task.replace_identical = True

for mesh_path in meshes_to_export:
    asset = unreal.EditorAssetLibrary.load_asset(mesh_path)
    export_task.object = asset
    export_task.filename = f"D:/GameDev/AssetPipeline/exchange/{asset.get_name()}.fbx"
    unreal.Exporter.run_asset_export_task(export_task)
Importing Finished Assets Back
After Blender creates a combined, retextured mesh, the UE MCP imports it:
task = unreal.AssetImportTask()
task.filename = "D:/GameDev/AssetPipeline/retextured/ReactorCorridor_Module_01.fbx"
task.destination_path = "/Game/CustomAssets/Modules/"
task.automated = True
task.replace_existing = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

3.4 Blender MCP: Mesh Fabrication & Kit-Bashing
Blender is where individual assets become unified game-ready meshes. The Blender MCP runs Python scripts inside Blender, giving Claude full access to mesh operations that UE5 simply doesn’t support. License: MIT. Cost: free.
Why Blender, Not UE5, For Mesh Combining
Unreal Engine is a runtime engine, not a Digital Content Creation (DCC) tool. It can group actors and merge for optimization, but it cannot:
Join multiple meshes into a single object with unified geometry
Perform boolean operations (union, difference, intersection) on meshes
Remesh and retopologize combined geometry for clean edge flow
Generate proper UV maps for a newly combined shape
Export a single combined mesh as a reusable asset
Blender does all of this natively, and the MCP exposes these operations through Python scripting.
Core Kit-Bashing Operations
Operation
Blender Python
Use Case
Import FBX
bpy.ops.import_scene.fbx(filepath=...)
Bring in assets from the exchange folder
Join Objects
bpy.ops.object.join()
Fuse multiple meshes into one object
Boolean Union
bpy.ops.object.modifier_add(type='BOOLEAN')
Combine overlapping shapes cleanly
Remesh
bpy.ops.object.modifier_add(type='REMESH')
Clean up topology after boolean operations
Auto UV Unwrap
bpy.ops.uv.smart_project()
Generate UVs for the combined mesh
Decimate
bpy.ops.object.modifier_add(type='DECIMATE')
Reduce polycount for LOD generation
Export FBX
bpy.ops.export_scene.fbx(filepath=...)
Send combined mesh back to exchange folder

Example Kit-Bash Workflow Prompt
A single Claude prompt triggers a multi-step Blender MCP workflow:
"Import SM_Pipe_Cluster_01.fbx, SM_Wall_Metal_Panel.fbx, and SM_Console_Monitor.fbx
 from the exchange folder. Arrange the wall panel as a backdrop (3m wide, 4m tall),
 attach the pipe cluster to the upper-left corner, place the console at floor level
 centered on the wall. Join all objects into a single mesh called
 'ReactorCorridor_Module_01', generate UVs, and export as FBX to the exchange folder."

3.5 AI Texturing: Free & Local Pipeline
Once Blender produces a combined mesh, it needs textures that match your game’s visual identity. The entire texturing pipeline runs locally on your GPU with zero ongoing costs. This section covers every layer: painting style onto meshes, generating PBR maps, and creating tileable environment materials.
ALL TOOLS IN THIS SECTION ARE FREE
No API keys. No subscriptions. No per-generation fees. No cloud dependency.
Minimum hardware: NVIDIA GPU with 8 GB VRAM (SDXL). Recommended: 12+ GB VRAM.
AMD/Intel GPUs are not supported by most tools — CUDA is required.

Layer 1: Painting Style Onto Meshes — StableGen (Recommended Primary)
StableGen is a GPL-3.0 Blender 4.2+ addon that connects to a local ComfyUI backend and textures all mesh objects in a scene simultaneously from multiple camera viewpoints. It is the single best free tool for the kit-bash retexturing use case.
Key capabilities for your pipeline:
Scene-wide texturing: paints all objects in the scene at once, maintaining visual coherence across your kit-bashed assembly
IPAdapter style lock: feed a single reference image that defines your visual style, and every viewpoint render stays consistent
ControlNet awareness: uses Depth, Canny, and Normal maps simultaneously so the AI respects your mesh geometry
Sequential Mode: viewpoint-by-viewpoint painting with inpainting masks for seam-free results
Bake to UVs: converts multi-projection materials into standard UV-mapped texture images for export
SDXL and FLUX.1-dev support: SDXL needs 8 GB VRAM, FLUX needs 16 GB+
Installation: Install ComfyUI locally, download an SDXL checkpoint (e.g., Juggernaut XL), install StableGen from GitHub, point it at your ComfyUI instance. Full setup takes about 30 minutes.
GitHub: github.com/sakalond/StableGen (544+ stars, updated December 2025, active development)
Layer 1 Alternative: DiffusedTexture Addon
If ComfyUI feels like too much overhead, DiffusedTexture bundles PyTorch and Diffusers directly into Blender’s Python environment — no separate backend needed. It textures meshes via parallel multi-view projection with LoRA and IPAdapter support. Less feature-rich than StableGen but dramatically simpler to set up. Requires a CUDA GPU with 4+ GB VRAM. Early-stage (v0.1.0) but actively developed. License: open-source.
GitHub: github.com/FrederikHasecke/diffused-texture-addon
Layer 1 Alternative: Dream Textures
The most established option with 8,100+ GitHub stars (GPL-3.0). Runs Stable Diffusion locally in Blender for text-to-texture and depth-to-image scene texturing. However, development has effectively stalled since mid-2023, so it may have compatibility issues with Blender 4.x+. Best used as a lightweight fallback for quick texture sketches rather than production texturing.
GitHub: github.com/carson-katri/dream-textures


Layer 2: Generating PBR Maps From Painted Surfaces
StableGen paints albedo/color onto your mesh. To get a full PBR material (normal, roughness, metallic, height), you need a second tool to decompose or generate those additional maps.

Tool
What It Does
Input
Output
VRAM
License
Material Anything
Full PBR generation on 3D meshes via diffusion
Any 3D mesh (OBJ/FBX/GLB)
Albedo + Normal + Roughness + Metallic (UV-mapped)
12+ GB
MIT
MatForger
Text-to-PBR tileable material generation
Text prompt
Basecolor + Normal + Height + Roughness + Metallic
8+ GB
OpenRAIL
CHORD (Ubisoft)
Decomposes any texture into PBR channels
Any albedo/color image
Base Color + Normal + Height + Roughness + Metalness
~8 GB
Research-Only*
DeepBump
Normal & height map from color image
Any color texture image
Normal map + Height map
CPU only
GPL
PBRFusion
Portable PBR map generation
Any albedo texture
Normal + Height + Roughness
~8 GB
Open (HuggingFace)

*CHORD License Warning: Ubisoft’s CHORD carries a Research-Only license. Review terms before shipping commercially. For a safe commercial pipeline, use Material Anything (MIT) or MatForger (OpenRAIL) instead.

Recommended: Material Anything (CVPR 2025 Highlight)
Material Anything is the strongest free tool for generating PBR materials on existing meshes. It takes any 3D object — texture-less, albedo-only, scanned, or AI-generated — and outputs UV-ready albedo, roughness, metallic, and normal maps using a triple-head diffusion architecture. It handles varying lighting conditions automatically and includes a UV-space refiner for clean results.
MIT-licensed with code, weights, and an 80K-object training dataset all publicly available. Requires 12+ GB VRAM (tested on RTX 3090/4090) and uses Blender 3.2.2+ for its rendering pipeline. Installation requires PyTorch3D compilation — expect some setup friction, but the output quality is production-grade.
GitHub: github.com/3DTopia/MaterialAnything
Lightweight Alternative: DeepBump
If you don’t need full PBR and just want quick normal and height maps from a color texture, DeepBump is a free Blender addon that runs on CPU. No GPU required, instant results, GPL licensed. Perfect for quick iterations before committing to a full Material Anything pass.
GitHub: github.com/HugoTini/DeepBump


Layer 3: Tileable Environment Textures
For walls, floors, terrain, and other surfaces that tile across large areas, you need a different workflow than mesh painting. Generate seamless tileable albedo, then decompose into PBR channels.
Step 1 — Generate Tileable Albedo: Use ComfyUI with SDXL and the "Tiling" latent option enabled, or install the Flux Seamless Texture LoRA (trigger word: "smlstxtr") for higher quality results. Both produce seamless textures from text prompts at zero cost.
Step 2 — Decompose to PBR: Run the generated albedo through CHORD (for research/prototyping) or MatForger (for commercial-safe output) to get the full PBR map set.
Step 3 — Polish: Use Material Maker (free, MIT-licensed, Substance Designer alternative built on Godot) for procedural adjustments, weathering, and wear effects.
Material Maker is worth highlighting specifically: it’s a fully free procedural texture tool with 250+ nodes that creates inherently tileable PBR materials with exports for UE5, Unity, and Godot. Think of it as a free Substance Designer. Download: materialmaker.org


Layer 4: ComfyUI-BlenderAI-Node (Power User Integration)
For maximum flexibility, the ComfyUI-BlenderAI-node addon brings full ComfyUI node workflows directly into Blender’s node editor. You can build custom pipelines that chain generation, PBR extraction, and baking in a single workflow. Actively maintained (v1.51+) but has a steep learning curve. License: open-source. This is optional but powerful for later optimization of your pipeline.
GitHub: github.com/AIGODLIKE/ComfyUI-BlenderAI-node

3.6 Style Prompt System
Visual consistency across dozens of AI-generated textures requires a disciplined prompt system. Save a master style prompt template that every texturing step references.
Master Style Template
# style-prompts/europa_industrial.txt

Dark industrial science fiction. Matte metal surfaces with subtle blue-teal
accent lighting embedded in seams and edges. Worn and weathered with micro-
scratching and subtle grime accumulation in recessed areas. Color palette
strictly restricted to: dark gunmetal grays (#2A2D35), deep blues (#1B2838),
cold whites (#C8D0D8), and teal accents (#4A9EBF). No warm colors. No bright
or saturated hues. Inspired by Blade Runner 2049 interiors, Alien: Isolation
station corridors, and SOMA underwater facility aesthetic. PBR-ready with
high roughness (0.7-0.9), low metallic on painted surfaces, high metallic on
exposed metal. Normal maps should emphasize panel lines and bolt details.

Use this template as the base prompt for every StableGen session. Feed a matching reference image through IPAdapter to lock the style visually. Save location-specific variants (Europa, Babylon, Citadels) in the style-prompts/ folder.

4. Implementation Roadmap
Build this incrementally. Each phase is independently useful, and you don’t need the full pipeline to start getting value.
Phase 1: Foundation (Week 1–2)
Goal: Searchable catalog + manual asset workflow
8. Install UEVaultManager (pip install UEVaultManager), authenticate with your Epic account
9. Run initial catalog scan to build SQLite database of all owned assets
10. Create folder structure (AssetPipeline/catalog, exchange, retextured, style-prompts)
11. Write the asset-manifest.md file manually for your 20–30 most-used asset packs with custom tags
12. Verify Blender MCP + UE5 MCP are both running and Claude can reach them simultaneously
13. Test the manual pipeline: UEVaultManager download → UE export FBX → Blender import → join → export → UE import
PHASE 1 EXIT CRITERIA
✓ SQLite catalog contains all owned assets with metadata
✓ Manifest file covers top 20–30 packs with custom universe tags
✓ Successfully completed one full manual pipeline cycle (download → combine → reimport)
✓ Both MCP servers confirmed working in same Claude session

Phase 2: MCP Integration (Week 3–4)
Goal: Claude-orchestrated asset selection and placement
14. Build the custom Asset Library MCP server with search_assets, check_downloaded, and download_asset tools
15. Add custom_tags table to the SQLite database and populate for your key asset packs
16. Write and test UE5 Python export/import scripts through the UE MCP’s Python execution capability
17. Test end-to-end: ask Claude to “build a corridor module from my library” and verify it chains all MCP calls
PHASE 2 EXIT CRITERIA
✓ Custom MCP server running with search, download, and list_meshes tools
✓ Claude can search your catalog, download a missing pack, and confirm it’s available
✓ UE5 MCP successfully round-trips meshes (export → exchange folder → reimport)

Phase 3: Local AI Texturing (Week 5–7)
Goal: Automated kit-bashing with free local AI retexturing
18. Install ComfyUI locally with an SDXL checkpoint (Juggernaut XL recommended), ControlNet models, and IPAdapter
19. Install StableGen addon in Blender 4.2+, connect to local ComfyUI, test on a simple cube
20. Write your master style prompt template and create a reference image for IPAdapter style-locking
21. Install Material Anything (requires PyTorch3D compilation) and test PBR generation on a simple mesh
22. Install DeepBump addon for quick normal/height generation as a lightweight alternative
23. Build a modular kit: Create 5–10 reusable corridor/room modules using the full pipeline
24. Document what works: note which prompts and IPAdapter images produce the best results
PHASE 3 EXIT CRITERIA
✓ Full pipeline works end-to-end: catalog → download → kit-bash → AI texture → PBR maps → UE5 import
✓ 5–10 reusable modules created with consistent visual style
✓ Style prompt + IPAdapter reference producing repeatable results
✓ No paid tools or API keys used anywhere in the pipeline

Phase 4: Pipeline Optimization (Week 8+)
Goal: ComfyUI MCP integration + tileable material library
25. Install ComfyUI MCP server (PurlieuStudios/comfyui-mcp or lalanikarim/comfy-mcp-server) so Claude can trigger texture generation directly
26. Build tileable material library: use ComfyUI + Flux Seamless Texture LoRA + MatForger for walls, floors, metals, concrete
27. Add LOD generation to the Blender pipeline (Decimate modifier at multiple levels)
28. Add collision mesh generation in Blender before UE5 reimport
29. Explore ComfyUI-BlenderAI-node for integrated generation → PBR → bake workflows
30. Install Material Maker for procedural material authoring and weathering effects
PHASE 4 EXIT CRITERIA
✓ ComfyUI MCP server running — Claude can generate textures on demand
✓ 20+ tileable PBR materials in your library covering key environment types
✓ Full pipeline orchestrated from a single Claude conversation window

5. MCP Configuration Reference
Complete MCP server configuration for the full pipeline. Every server listed is free and open-source.
Full Config Block
{
  "mcpServers": {

    "blender": {
      "command": "uvx",
      "args": ["blender-mcp"]
    },

    "unrealMCP": {
      "command": "uv",
      "args": [
        "--directory", "D:/Tools/unreal-engine-mcp/Python",
        "run", "unreal_mcp_server_advanced.py"
      ]
    },

    "asset-library": {
      "command": "python",
      "args": ["D:/GameDev/AssetPipeline/asset_library_mcp.py"]
    },

    "comfyui": {
      "command": "python",
      "args": ["D:/Tools/comfyui-mcp-server/server.py"],
      "env": { "COMFYUI_URL": "http://127.0.0.1:8188" }
    },

    "filesystem": {
      "command": "npx",
      "args": [
        "-y", "@anthropic/mcp-filesystem",
        "D:/GameDev/AssetPipeline",
        "D:/GameDev/UE5Projects/WarpBorn"
      ]
    }

  }
}

Verification Commands
# In Claude Code
claude mcp list                    # Shows all configured servers
/mcp                               # Inside a session, shows connection status

# Quick test prompts
"List all tools available from the asset-library MCP"
"Ask Blender to create a cube"
"Ask UE5 to list all actors in the current level"
"Generate a seamless metal texture via ComfyUI"

6. Complete Tool Stack
Every tool in this pipeline is free, open-source, or custom-built. The following table is the authoritative reference for the full stack.
6.1 Core Pipeline Tools
Tool
Purpose
License
VRAM
URL
UEVaultManager
Vault catalog + asset download CLI
GPL-based (OSS)
None
github.com/LaurentOngaro/UEVaultManager
Blender MCP
Natural language Blender control
MIT
None
github.com/ahujasid/blender-mcp
Flopperam UE5 MCP
Natural language UE5 control
OSS
None
github.com/flopperam/unreal-engine-mcp
Custom Asset Library MCP
Your own catalog bridge server
Yours
None
You build this (Python + MCP SDK)
Filesystem MCP
Secure file read/write for Claude
MIT (Anthropic)
None
Official Anthropic MCP server
GitHub MCP
Version control from Claude
MIT (Anthropic)
None
Official Anthropic MCP server

6.2 AI Texturing Tools (All Free & Local)
Tool
Purpose
License
VRAM
URL
ComfyUI
AI pipeline backend (StableGen needs this)
GPL-3.0
8+ GB
github.com/comfyanonymous/ComfyUI
StableGen
Multi-view AI texture painting in Blender
GPL-3.0
8+ GB
github.com/sakalond/StableGen
Material Anything
Full PBR map generation on meshes
MIT
12+ GB
github.com/3DTopia/MaterialAnything
MatForger
Text-to-PBR tileable materials
OpenRAIL
8+ GB
huggingface.co/gvecchio/MatForger
DeepBump
Normal/height maps from color textures
GPL
CPU
github.com/HugoTini/DeepBump
CHORD (Ubisoft)
PBR decomposition from any texture
Research*
~8 GB
Ubisoft LaForge (SIGGRAPH Asia 2025)
PBRFusion
Portable PBR map generator
Open (HF)
~8 GB
huggingface.co/NightRaven109/PBRFusion
Flux Seamless LoRA
Tileable texture generation from prompts
Open (HF)
12+ GB
huggingface.co/gokaygokay/Flux-Seamless-Texture-LoRA
DiffusedTexture
Simpler Blender texture addon (no ComfyUI)
OSS
4+ GB
github.com/FrederikHasecke/diffused-texture-addon
Dream Textures
Stable Diffusion in Blender (legacy)
GPL-3.0
4+ GB
github.com/carson-katri/dream-textures

6.3 Material & Procedural Tools (All Free)
Tool
Purpose
License
URL
Material Maker
Free Substance Designer alternative (procedural PBR)
MIT
materialmaker.org
ArmorLab
AI-augmented standalone texture tool (source open, binary $19)
Source: open
armorlab.org
Poly Haven
Free CC0 textures, HDRIs, and models
CC0
polyhaven.com
ComfyUI-TextureAlchemy
PBR processing nodes for ComfyUI
OSS
github.com/amtarr/ComfyUI-TextureAlchemy
ComfyUI-BlenderAI-node
Full ComfyUI workflows inside Blender
OSS
github.com/AIGODLIKE/ComfyUI-BlenderAI-node

6.4 MCP Servers for Texture Generation
Server
Purpose
License
URL
ComfyUI MCP (PurlieuStudios)
Game dev focused — environment textures, asset pipeline
MIT
github.com/PurlieuStudios/comfyui-mcp
ComfyUI MCP (lalanikarim)
Lightweight, supports Ollama for local LLM
MIT
github.com/lalanikarim/comfy-mcp-server
ComfyUI MCP (joenorton)
Simple, supports iterative refinement
MIT
github.com/joenorton/comfyui-mcp-server
Blender Open MCP
Blender control via Ollama (fully local AI)
OSS
github.com/dhakalnirajan/blender-open-mcp

7. Hardware Requirements
The free pipeline trades subscription costs for GPU compute. Here is what you need at each tier:
GPU Tier
VRAM
What You Can Run
Example Cards
Minimum
8 GB
ComfyUI + SDXL, StableGen (SDXL mode), DeepBump, MatForger, CHORD
RTX 3060 8GB, RTX 4060
Recommended
12 GB
All of the above + Material Anything, PBRFusion, larger batch sizes
RTX 3060 12GB, RTX 4070
Ideal
16–24 GB
All of the above + FLUX.1-dev, Hunyuan3D-2.1 Paint, faster generation
RTX 4070 Ti Super, RTX 4090

Critical: NVIDIA CUDA GPU is required. AMD and Intel GPUs are not supported by most tools in this pipeline without significant workarounds. If your current GPU falls short, the Tripo AI free tier (600 credits/month, cloud-based PBR output, CC BY 4.0 license on outputs) serves as a zero-cost fallback.

8. Risks & Mitigations
Risk
Likelihood
Impact
Mitigation
Epic API changes break UEVaultManager
Medium
High
Pin version, monitor GitHub issues, maintain local catalog backup
Epic captcha blocks automated downloads
Medium
Medium
Pre-download most-used packs, use on-demand only for rare assets
UE→FBX export loses material data
High
Low
Expected — AI retexturing pipeline replaces all materials anyway
Blender boolean operations produce bad topology
Medium
Medium
Add remesh step after booleans, prefer join over booleans when possible
StableGen/ComfyUI texture quality inconsistent
Medium
Medium
Use IPAdapter + strict style prompt template, iterate before committing
CHORD license incompatible with commercial release
Medium
High
Use Material Anything (MIT) or MatForger (OpenRAIL) for production
Material Anything setup fails (PyTorch3D compilation)
Medium
Medium
Fall back to DeepBump (CPU, zero-setup) + manual roughness/metallic in Blender
Pipeline too slow for rapid iteration
Low
High
Pre-build module library of 20–30 common pieces, pipeline only for new pieces
MCP servers conflict or crash
Low
Medium
Run each server independently, restart failed servers via /mcp command
Asset license violations from Marketplace
Low
Critical
Only use assets from your own Vault, verify per-pack license terms
GPU VRAM insufficient for full pipeline
Low
High
Use DiffusedTexture (4GB min) instead of StableGen, DeepBump instead of Material Anything

9. Glossary
Term
Definition
MCP
Model Context Protocol — standard for connecting AI assistants to external tools
Kit-bashing
Combining multiple 3D models into a single new asset by arranging and merging geometry
PBR
Physically Based Rendering — texture workflow using albedo, normal, roughness, metallic maps
DCC
Digital Content Creation tool (Blender, Maya, 3ds Max) as opposed to a game engine
.uasset
Unreal Engine’s proprietary binary asset format; cannot be opened in other tools directly
FBX
Autodesk interchange format; the standard bridge between DCC tools and game engines
Vault / Fab Library
Your collection of purchased/claimed assets in the Epic Games ecosystem
LOD
Level of Detail — lower-polycount versions of a mesh used at greater camera distances
ComfyUI
Node-based local AI pipeline tool; backend for StableGen and texture generation workflows
SDXL
Stable Diffusion XL — open-source image generation model, basis for most texture tools
IPAdapter
Image Prompt Adapter — feeds a reference image to lock style during AI generation
ControlNet
Neural network that adds geometric awareness (depth, normals, edges) to AI image generation
LoRA
Low-Rank Adaptation — small model fine-tune that adds specific capabilities (e.g., seamless tiling)
SVBRDF
Spatially Varying Bidirectional Reflectance Distribution Function — the math behind PBR materials


End of Document — All tools free & open-source