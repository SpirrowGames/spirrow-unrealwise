# ========================================
# Spirrow-UnrealWise MCP Server (SSE Mode)
# ========================================
#
# SSEモード: MCPサーバーを独立したHTTPサーバーとして起動
# メリット: Python変更時にClaude Desktopの再起動が不要
#

Write-Host "========================================"
Write-Host "  Unreal MCP Server (SSE Mode)"
Write-Host "========================================"
Write-Host ""

# [1/4] Load configuration
Write-Host "[1/4] Loading configuration..."
$configPath = Join-Path $PSScriptRoot "config.local.ps1"
if (Test-Path $configPath) {
    . $configPath
    Write-Host "      Loaded: config.local.ps1"
} else {
    Write-Host "      No config.local.ps1 found (using defaults)"
    if (-not $env:RAG_SERVER_URL) {
        $env:RAG_SERVER_URL = "http://localhost:8100"
    }
}
Write-Host "      RAG_SERVER_URL: $env:RAG_SERVER_URL"
Write-Host "      OK"
Write-Host ""

# [2/4] Change to Python directory
Write-Host "[2/4] Changing to Python directory..."
$pythonDir = Join-Path $PSScriptRoot "Python"
Set-Location $pythonDir
Write-Host "      Directory: $(Get-Location)"
Write-Host "      OK"
Write-Host ""

# [3/4] Check environment
Write-Host "[3/4] Checking environment..."
try {
    $uvVersion = uv --version 2>&1
    Write-Host "      uv: $uvVersion"
    Write-Host "      OK"
} catch {
    Write-Host "      ERROR: uv is not installed"
    Write-Host "      Please install uv: https://docs.astral.sh/uv/"
    Read-Host "Press Enter to exit"
    exit 1
}
Write-Host ""

# [4/4] Start MCP server in SSE mode
Write-Host "[4/4] Starting MCP Server (SSE Mode)..."
Write-Host ""
Write-Host "========================================"
Write-Host "  MCP SERVER IS NOW RUNNING (SSE)"
Write-Host "========================================"
Write-Host ""
Write-Host "Endpoint: http://localhost:8000/sse"
Write-Host ""
Write-Host "To connect Claude Desktop, use this config:"
Write-Host '  {'
Write-Host '    "mcpServers": {'
Write-Host '      "spirrow-unrealwise": {'
Write-Host '        "url": "http://localhost:8000/sse"'
Write-Host '      }'
Write-Host '    }'
Write-Host '  }'
Write-Host ""
Write-Host "Press Ctrl+C to stop the server"
Write-Host ""

uv run unreal_mcp_server.py --sse

Write-Host ""
Write-Host "========================================"
Write-Host "  MCP Server Stopped"
Write-Host "========================================"
