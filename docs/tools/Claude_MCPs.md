Tier 1 — Install immediately (day one tools)
Blender MCP (github.com/ahujasid/blender-mcp, 16,300+ stars) is the flagship. It lets Claude create and modify 3D objects, apply materials, set up lighting, import Poly Haven assets (free HDRIs, textures, models), and even generate 3D models from reference images via Hyper3D integration. For your pipeline — prototyping low-poly game assets, setting up scenes, learning Blender through natural language — this is essential. Install the addon in Blender, add the MCP config to Claude Desktop, and you can say things like "create a low-poly sci-fi corridor with blue accent lighting" and iterate from there. One important caveat: the results are starting points, not game-ready assets. You'll still need to clean topology, optimize poly counts, and UV unwrap manually for UE5 import. But for rapid prototyping and learning Blender's interface, it's a genuine accelerator.
Filesystem MCP (official Anthropic server) gives Claude secure read/write access to directories you specify. Essential for having Claude help you organize project files, write scripts, generate config files, or batch-process assets. Scope it to your project directories only.
GitHub MCP (official Anthropic server) lets Claude manage your repos — commits, branches, PRs, issue tracking. You should be version-controlling everything from day one, and this makes it frictionless to manage from within Claude conversations.

Tier 2 — Install when you start UE5 development (month 1–3)
Unreal Engine MCP — there are several competing options, each with different strengths:
The most advanced is Flopperam's unreal-engine-mcp (github.com/flopperam/unreal-engine-mcp), which now supports UE5 5.5–5.7, can generate entire environments from prompts, program Blueprints through AI including node creation and graph management, and has built-in text/image-to-3D generation. It recently added Blueprint Analysis tools that can inspect your Blueprints' variables, functions, and event dispatchers — meaning Claude can actually read and explain your Blueprint graphs. This is the one I'd prioritize.
Chongdashu's unreal-mcp (github.com/chongdashu/unreal-mcp) is simpler but stable — good for creating/deleting actors, setting transforms, querying properties, and running Python scripts in the editor. Comes with a UE5.5 starter project. Better if you want something lightweight.
Unreal Analyzer MCP (github.com/ayeletstudioindia/unreal-analyzer-mcp) is different — it's a source code analysis tool that helps Claude deeply understand UE5's C++ codebase. Useful when you need to understand how an engine subsystem works or debug inheritance hierarchies. More relevant once you start touching C++ or need to understand plugin internals.
All of these connect through UE5's Remote Control API plugin, which you'll need to enable in your project settings.

Tier 3 — Useful but situational
Godot MCP (multiple options — GDAI MCP at gdaimcp.com is the most polished) if you decide to prototype smaller stepping-stone games in Godot before committing fully to UE5. It can create scenes, manage nodes, read errors, run projects, and even take screenshots of the running game for Claude to analyze. Good for rapid game jam entries.
Sequential Thinking MCP — helps Claude approach complex multi-step problems methodically rather than jumping to solutions. Useful when you're planning architecture decisions or debugging intricate Blueprint logic.
Memory Bank MCP — persistent context across Claude sessions. Helpful when you're working on a long-running project and want Claude to retain knowledge about your codebase architecture, naming conventions, and design decisions between conversations.

What's NOT worth installing yet
Don't bother with database MCPs (Postgres, SQLite), cloud provider MCPs (AWS, GCP), or web automation MCPs (Playwright, Puppeteer) — they're powerful but irrelevant to your workflow right now. Keep your config lean. Every MCP server adds latency and context overhead.

Practical setup order:

Install Claude Desktop (MCP only works in the desktop app, not web)
Add Filesystem + GitHub MCPs
Install Blender + Blender MCP addon
When UE5 work begins, add Flopperam's Unreal MCP
Add others as needed

Your claude_desktop_config.json will end up looking something like:
json{
  "mcpServers": {
    "blender": {
      "command": "uvx",
      "args": ["blender-mcp"]
    },
    "filesystem": {
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-filesystem", "/path/to/your/projects"]
    },
    "github": {
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-github"],
      "env": { "GITHUB_PERSONAL_ACCESS_TOKEN": "your-token" }
    }
  }
}
One reality check: AI-generated 3D meshes through Blender MCP are currently best for blocking out environments and creating placeholder assets, not final game art. The topology tends to be messy and non-optimized. Your workflow will likely be: Claude generates a rough shape → you manually retopologize and UV unwrap → export to UE5. That's still dramatically faster than starting from zero in Blender as a non-artist.

***Not using Godot. At least not yet I don't think. Everything in UE.