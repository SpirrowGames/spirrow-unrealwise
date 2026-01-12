"""
AI Image Generation Tools for Unreal MCP.

This module provides tools for generating images using Stable Diffusion Forge API
and importing them as textures into Unreal Engine.
"""

import logging
import os
import json
from typing import Dict, Any, Optional
import requests
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("SpirrowBridge")

# Get AI Image server URL from environment variable or use default
AI_IMAGE_SERVER_URL = os.environ.get("AI_IMAGE_SERVER_URL", "http://localhost:7860")

# Default timeout for image generation (can be long for high-step counts)
GENERATION_TIMEOUT = 120  # 2 minutes

# Preset configurations for common use cases
PRESETS = {
    "game_icon": {
        "width": 512,
        "height": 512,
        "steps": 25,
        "cfg_scale": 7.5,
        "negative_prompt_suffix": ", text, watermark, signature"
    },
    "texture_tileable": {
        "width": 1024,
        "height": 1024,
        "steps": 30,
        "cfg_scale": 7.0,
        "negative_prompt_suffix": ", text, watermark, seams visible"
    },
    "concept_art": {
        "width": 768,
        "height": 512,
        "steps": 35,
        "cfg_scale": 8.0,
        "negative_prompt_suffix": ", blurry, low quality"
    },
    "character_portrait": {
        "width": 512,
        "height": 768,
        "steps": 30,
        "cfg_scale": 7.5,
        "negative_prompt_suffix": ", deformed, ugly, blurry"
    }
}


def generate_image_internal(
    prompt: str,
    negative_prompt: str = "",
    width: int = 512,
    height: int = 512,
    steps: int = 20,
    cfg_scale: float = 7.0,
    seed: int = -1,
    sampler_name: str = "Euler"
) -> Dict[str, Any]:
    """
    Internal synchronous function to generate an image.
    Can be called from other tools without async context.

    Returns:
        Dict with success status, image_base64, seed, and parameters
    """
    try:
        url = f"{AI_IMAGE_SERVER_URL}/sdapi/v1/txt2img"
        payload = {
            "prompt": prompt,
            "negative_prompt": negative_prompt,
            "width": width,
            "height": height,
            "steps": steps,
            "cfg_scale": cfg_scale,
            "seed": seed,
            "sampler_name": sampler_name
        }

        logger.info(f"Generating image with prompt: '{prompt[:50]}...' ({width}x{height}, {steps} steps)")

        response = requests.post(url, json=payload, timeout=GENERATION_TIMEOUT)
        response.raise_for_status()

        data = response.json()

        # Extract the first image (Forge returns array of images)
        images = data.get("images", [])
        if not images:
            return {"success": False, "message": "No images returned from server"}

        # Get the actual seed used (important for reproducibility)
        info = data.get("info", "{}")
        if isinstance(info, str):
            try:
                info = json.loads(info)
            except json.JSONDecodeError:
                info = {}

        actual_seed = info.get("seed", seed)

        logger.info(f"Image generated successfully (seed: {actual_seed})")

        return {
            "success": True,
            "image_base64": images[0],
            "seed": actual_seed,
            "parameters": {
                "prompt": prompt,
                "negative_prompt": negative_prompt,
                "width": width,
                "height": height,
                "steps": steps,
                "cfg_scale": cfg_scale,
                "sampler_name": sampler_name
            }
        }

    except requests.exceptions.ConnectionError as e:
        error_msg = f"Failed to connect to AI image server at {AI_IMAGE_SERVER_URL}"
        logger.error(f"{error_msg}: {e}")
        return {
            "success": False,
            "message": error_msg,
            "hint": "Make sure AI image server is running and AI_IMAGE_SERVER_URL is correct"
        }
    except requests.exceptions.Timeout as e:
        error_msg = "AI image server request timed out"
        logger.error(f"{error_msg}: {e}")
        return {
            "success": False,
            "message": error_msg,
            "hint": "Try reducing steps or image size"
        }
    except requests.exceptions.HTTPError as e:
        logger.error(f"HTTP error from AI server: {e}")
        try:
            error_detail = e.response.json().get("detail", str(e))
        except Exception:
            error_detail = str(e)
        return {"success": False, "message": f"AI server error: {error_detail}"}
    except Exception as e:
        error_msg = f"Error generating image: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}


def register_image_gen_tools(mcp: FastMCP):
    """Register AI image generation tools with the MCP server."""

    @mcp.tool()
    def get_ai_image_server_status(ctx: Context) -> Dict[str, Any]:
        """
        Check the status of the AI image generation server (Stable Diffusion Forge).

        Returns:
            Dict containing:
            - success: Whether the server is reachable
            - server_url: The configured server URL
            - status: "online" or "offline"
            - models: List of available models (if server is up)
            - samplers: List of available samplers (if server is up)
            - message: Status message

        Example:
            get_ai_image_server_status()
        """
        try:
            # Check models endpoint
            models_url = f"{AI_IMAGE_SERVER_URL}/sdapi/v1/sd-models"
            samplers_url = f"{AI_IMAGE_SERVER_URL}/sdapi/v1/samplers"

            logger.info(f"Checking AI image server status at {AI_IMAGE_SERVER_URL}")

            models_response = requests.get(models_url, timeout=5)
            models_response.raise_for_status()
            models_data = models_response.json()
            models = [m.get("model_name", m.get("title", "unknown")) for m in models_data]

            samplers_response = requests.get(samplers_url, timeout=5)
            samplers_response.raise_for_status()
            samplers_data = samplers_response.json()
            samplers = [s.get("name", "unknown") for s in samplers_data]

            return {
                "success": True,
                "server_url": AI_IMAGE_SERVER_URL,
                "status": "online",
                "models": models[:10],  # Limit to first 10 for readability
                "model_count": len(models),
                "samplers": samplers,
                "message": f"AI image server is online with {len(models)} models available"
            }

        except requests.exceptions.ConnectionError:
            return {
                "success": False,
                "server_url": AI_IMAGE_SERVER_URL,
                "status": "offline",
                "message": f"Cannot connect to AI image server at {AI_IMAGE_SERVER_URL}",
                "hint": "Make sure Stable Diffusion Forge is running with --api flag"
            }
        except requests.exceptions.Timeout:
            return {
                "success": False,
                "server_url": AI_IMAGE_SERVER_URL,
                "status": "timeout",
                "message": "AI image server request timed out"
            }
        except Exception as e:
            return {
                "success": False,
                "server_url": AI_IMAGE_SERVER_URL,
                "status": "error",
                "message": str(e)
            }

    @mcp.tool()
    def generate_image(
        ctx: Context,
        prompt: str,
        negative_prompt: str = "",
        width: int = 512,
        height: int = 512,
        steps: int = 20,
        cfg_scale: float = 7.0,
        seed: int = -1,
        sampler_name: str = "Euler",
        preset: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Generate an image using the AI image server (Stable Diffusion Forge).

        Args:
            prompt: Text description of the desired image
            negative_prompt: Things to avoid in the image (default: "")
            width: Image width in pixels (default: 512, must be multiple of 8)
            height: Image height in pixels (default: 512, must be multiple of 8)
            steps: Number of sampling steps (default: 20, range: 1-150)
            cfg_scale: Classifier-free guidance scale (default: 7.0, range: 1-30)
            seed: Random seed for reproducibility (-1 for random)
            sampler_name: Sampler algorithm (default: "Euler")
            preset: Optional preset name for common use cases:
                - "game_icon": 512x512, optimized for UI icons
                - "texture_tileable": 1024x1024, seamless texture settings
                - "concept_art": 768x512, for concept artwork
                - "character_portrait": 512x768, portrait orientation

        Returns:
            Dict containing:
            - success: Whether generation succeeded
            - image_base64: Base64-encoded PNG image data
            - seed: Seed used for generation (for reproducibility)
            - parameters: Generation parameters used

        Example:
            # Generate a game icon
            generate_image(
                prompt="sword icon, fantasy RPG, golden handle, detailed",
                negative_prompt="blurry, low quality",
                preset="game_icon"
            )

            # Generate with specific parameters
            generate_image(
                prompt="medieval castle at sunset, dramatic lighting",
                width=1024,
                height=768,
                steps=30,
                cfg_scale=8.5
            )
        """
        # Apply preset if specified
        if preset and preset in PRESETS:
            preset_config = PRESETS[preset]
            width = preset_config.get("width", width)
            height = preset_config.get("height", height)
            steps = preset_config.get("steps", steps)
            cfg_scale = preset_config.get("cfg_scale", cfg_scale)
            suffix = preset_config.get("negative_prompt_suffix", "")
            if suffix and suffix not in negative_prompt:
                negative_prompt = negative_prompt + suffix
            logger.info(f"Applied preset '{preset}': {width}x{height}, {steps} steps")
        elif preset:
            logger.warning(f"Unknown preset '{preset}', using default parameters. Available: {list(PRESETS.keys())}")

        # Validate parameters
        if width % 8 != 0 or height % 8 != 0:
            return {
                "success": False,
                "message": "Width and height must be multiples of 8",
                "hint": f"Got width={width}, height={height}"
            }

        if not (1 <= steps <= 150):
            return {
                "success": False,
                "message": "Steps must be between 1 and 150",
                "hint": f"Got steps={steps}"
            }

        return generate_image_internal(
            prompt=prompt,
            negative_prompt=negative_prompt,
            width=width,
            height=height,
            steps=steps,
            cfg_scale=cfg_scale,
            seed=seed,
            sampler_name=sampler_name
        )

    @mcp.tool()
    def generate_and_import_texture(
        ctx: Context,
        prompt: str,
        asset_name: str,
        destination_path: str = "/Game/Generated",
        negative_prompt: str = "",
        width: int = 512,
        height: int = 512,
        steps: int = 20,
        cfg_scale: float = 7.0,
        seed: int = -1,
        sampler_name: str = "Euler",
        preset: Optional[str] = None,
        compression: str = "Default",
        srgb: bool = True,
        lod_group: str = "World"
    ) -> Dict[str, Any]:
        """
        Generate an AI image and import it directly as a texture in Unreal Engine.

        This combines generate_image and import_texture into a single workflow.

        Args:
            prompt: Text description of the desired image
            asset_name: Name for the imported texture asset (e.g., "T_Generated_Sword")
            destination_path: Content browser path (default: "/Game/Generated")
            negative_prompt: Things to avoid in the image
            width: Image width in pixels (default: 512)
            height: Image height in pixels (default: 512)
            steps: Number of sampling steps (default: 20)
            cfg_scale: Guidance scale (default: 7.0)
            seed: Random seed (-1 for random)
            sampler_name: Sampler algorithm (default: "Euler")
            preset: Optional preset (see generate_image for options)
            compression: UE texture compression ("Default", "Normalmap", "Masks", "UI", "BC7")
            srgb: Whether texture is sRGB (default: True)
            lod_group: LOD group ("World", "WorldNormalMap", "UI", "Lightmap")

        Returns:
            Dict containing:
            - success: Whether the entire operation succeeded
            - generation: Generation result details (seed, parameters)
            - import: Import result details (asset_path)
            - message: Status message

        Example:
            # Generate and import a UI icon
            generate_and_import_texture(
                prompt="health potion icon, red liquid, glass bottle, fantasy",
                asset_name="T_Icon_HealthPotion",
                destination_path="/Game/UI/Icons",
                preset="game_icon",
                compression="UI"
            )

            # Generate tileable texture
            generate_and_import_texture(
                prompt="stone wall texture, medieval, seamless, tileable",
                asset_name="T_StoneWall_D",
                destination_path="/Game/Textures/Environment",
                preset="texture_tileable",
                compression="Default"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        # Apply preset if specified
        if preset and preset in PRESETS:
            preset_config = PRESETS[preset]
            width = preset_config.get("width", width)
            height = preset_config.get("height", height)
            steps = preset_config.get("steps", steps)
            cfg_scale = preset_config.get("cfg_scale", cfg_scale)
            suffix = preset_config.get("negative_prompt_suffix", "")
            if suffix and suffix not in negative_prompt:
                negative_prompt = negative_prompt + suffix
            logger.info(f"Applied preset '{preset}': {width}x{height}, {steps} steps")
        elif preset:
            logger.warning(f"Unknown preset '{preset}', using default parameters")

        # Validate parameters
        if width % 8 != 0 or height % 8 != 0:
            return {
                "success": False,
                "message": "Width and height must be multiples of 8",
                "hint": f"Got width={width}, height={height}"
            }

        logger.info(f"Generating and importing texture '{asset_name}' to '{destination_path}'")

        # Step 1: Generate the image
        gen_result = generate_image_internal(
            prompt=prompt,
            negative_prompt=negative_prompt,
            width=width,
            height=height,
            steps=steps,
            cfg_scale=cfg_scale,
            seed=seed,
            sampler_name=sampler_name
        )

        if not gen_result.get("success"):
            return {
                "success": False,
                "stage": "generation",
                "message": gen_result.get("message", "Image generation failed"),
                "hint": gen_result.get("hint")
            }

        # Step 2: Save base64 to temp file (avoids sending large data through socket)
        import base64
        import tempfile

        try:
            # Decode base64 and save to temp file
            image_data = base64.b64decode(gen_result["image_base64"])

            # Create temp file in project's Saved/Temp directory
            # Use a predictable path that Unreal can access
            temp_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), "Saved", "Temp")
            os.makedirs(temp_dir, exist_ok=True)
            temp_file_path = os.path.join(temp_dir, f"{asset_name}_generated.png")

            with open(temp_file_path, 'wb') as f:
                f.write(image_data)

            logger.info(f"Saved generated image to temp file: {temp_file_path} ({len(image_data)} bytes)")

        except Exception as e:
            return {
                "success": False,
                "stage": "file_save",
                "message": f"Failed to save generated image to temp file: {str(e)}",
                "generation": {
                    "success": True,
                    "seed": gen_result.get("seed"),
                    "hint": "Image was generated but couldn't be saved locally."
                }
            }

        # Step 3: Import to Unreal using file path (not base64)
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {
                    "success": False,
                    "stage": "import",
                    "message": "Failed to connect to Unreal Engine",
                    "generation": {
                        "success": True,
                        "seed": gen_result.get("seed"),
                        "temp_file": temp_file_path,
                        "hint": "Image saved to temp file. Use import_texture with the file path."
                    }
                }

            # Send file path instead of base64 data (much smaller payload)
            import_params = {
                "source": temp_file_path.replace("\\", "/"),  # Normalize path separators
                "asset_name": asset_name,
                "destination_path": destination_path,
                "source_type": "file",  # Use file instead of base64
                "compression": compression,
                "srgb": srgb,
                "lod_group": lod_group
            }

            logger.info(f"Importing generated texture to Unreal: {asset_name}")
            logger.info(f"generate_and_import_texture: Sending import_texture command (file path)...")
            import_response = unreal.send_command("import_texture", import_params)
            logger.info(f"generate_and_import_texture: Received response: {import_response}")

            if not import_response:
                return {
                    "success": False,
                    "stage": "import",
                    "message": "No response from Unreal Engine - Editor may have crashed",
                    "hint": "Check Unreal Editor logs in Saved/Logs/",
                    "generation": {
                        "success": True,
                        "seed": gen_result.get("seed")
                    }
                }

            # Check for error in response
            if import_response.get("status") == "error" or not import_response.get("success", True):
                error_msg = import_response.get("error", import_response.get("message", "Unknown import error"))
                return {
                    "success": False,
                    "stage": "import",
                    "message": f"Texture import failed: {error_msg}",
                    "generation": {
                        "success": True,
                        "seed": gen_result.get("seed")
                    }
                }

            logger.info(f"Texture imported successfully: {destination_path}/{asset_name}")

            # Clean up temp file after successful import
            try:
                if os.path.exists(temp_file_path):
                    os.remove(temp_file_path)
                    logger.info(f"Cleaned up temp file: {temp_file_path}")
            except Exception as cleanup_err:
                logger.warning(f"Failed to clean up temp file: {cleanup_err}")

            return {
                "success": True,
                "message": f"Successfully generated and imported texture '{asset_name}'",
                "generation": {
                    "seed": gen_result.get("seed"),
                    "parameters": gen_result.get("parameters")
                },
                "import": {
                    "asset_path": f"{destination_path}/{asset_name}",
                    "compression": compression,
                    "srgb": srgb,
                    "lod_group": lod_group
                }
            }

        except (ConnectionResetError, BrokenPipeError) as e:
            error_msg = f"Connection lost to Unreal Engine (Editor likely crashed): {e}"
            logger.error(error_msg)
            return {
                "success": False,
                "stage": "import",
                "message": error_msg,
                "hint": "Check Unreal Editor logs in Saved/Logs/ for crash details",
                "generation": {
                    "success": True,
                    "seed": gen_result.get("seed")
                }
            }
        except Exception as e:
            error_msg = f"Error importing texture: {e}"
            logger.error(error_msg)
            return {
                "success": False,
                "stage": "import",
                "message": error_msg,
                "generation": {
                    "success": True,
                    "seed": gen_result.get("seed")
                }
            }

    logger.info(f"AI Image Generation tools registered (AI_IMAGE_SERVER_URL: {AI_IMAGE_SERVER_URL})")
