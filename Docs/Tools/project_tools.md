---
id: spirrow-unrealwise:tools-project
title: Tool Docs Project & Enhanced Input Tools
product: spirrow-unrealwise
type: reference
status: active
version: 1.0
created: 2025-12-13
last_verified: 2026-09-10
supersedes: []
related: []
keywords: [ツール, Project, EnhancedInput, リファレンス]
legacy_drive_id: [1LY1KzMyxA1n9Q6FjDy_5oPIcT0XynBuGc8dCh_b04Dk]
---

# Project Tools

Project-wide tools for managing input systems and project settings.

## Enhanced Input System Tools

### `create_input_action`

Creates an Enhanced Input Action asset.

**Parameters:**
- `action_name` (str): Name of the input action (e.g., "IA_Jump")
- `value_type` (str, optional): Type of input value - "Digital", "Axis1D", "Axis2D", "Axis3D" (default: "Digital")
- `path` (str, optional): Content browser path for the asset (default: "/Game/Input")

**Returns:**
- Dictionary containing the created asset path and value type

**Example:**
```python
# Create a digital input action for jumping
result = create_input_action(
    action_name="IA_Jump",
    value_type="Digital",
    path="/Game/Input"
)

# Create a 2D input action for movement
result = create_input_action(
    action_name="IA_Move",
    value_type="Axis2D",
    path="/Game/Input"
)
```

**Natural Language Examples:**
- "Create a digital input action named IA_Jump in /Game/Input"
- "Make a 2D axis input action called IA_Move for player movement"

---

### `create_input_mapping_context`

Creates an Enhanced Input Mapping Context asset.

**Parameters:**
- `context_name` (str): Name of the mapping context (e.g., "IMC_Default")
- `path` (str, optional): Content browser path for the asset (default: "/Game/Input")

**Returns:**
- Dictionary containing the created asset path

**Example:**
```python
# Create a default input mapping context
result = create_input_mapping_context(
    context_name="IMC_Default",
    path="/Game/Input"
)
```

**Natural Language Examples:**
- "Create an input mapping context named IMC_Default in /Game/Input"
- "Make a new mapping context called IMC_Gameplay"

---

### `add_action_to_mapping_context`

Adds an Input Action mapping to an Input Mapping Context with triggers and modifiers.

**Parameters:**
- `context_name` (str): Name of the mapping context
- `action_name` (str): Name of the input action to map
- `key` (str): Key to bind (e.g., "SpaceBar", "LeftShift", "W", "A", "S", "D", "MouseX", "MouseY")
- `trigger_type` (str, optional): Trigger type - "Down", "Pressed", "Released", "Hold", "HoldAndRelease" (default: "Down")
- `modifiers` (list, optional): List of modifiers - "Negate", "SwizzleYXZ", "DeadZone", etc.
- `context_path` (str, optional): Path where the IMC asset is located (default: "/Game/Input")
- `action_path` (str, optional): Path where the IA asset is located (default: "/Game/Input")

**Returns:**
- Dictionary containing the mapping result with context, action, key, and trigger information

**Example:**
```python
# Map jump action to space bar with pressed trigger
result = add_action_to_mapping_context(
    context_name="IMC_Default",
    action_name="IA_Jump",
    key="SpaceBar",
    trigger_type="Pressed"
)

# Map movement to WASD keys
result = add_action_to_mapping_context(
    context_name="IMC_Default",
    action_name="IA_Move",
    key="W",
    trigger_type="Down"
)

# Map crouching with hold trigger
result = add_action_to_mapping_context(
    context_name="IMC_Default",
    action_name="IA_Crouch",
    key="LeftCtrl",
    trigger_type="Hold"
)

# Map backward movement with negate modifier
result = add_action_to_mapping_context(
    context_name="IMC_Default",
    action_name="IA_Move",
    key="S",
    trigger_type="Down",
    modifiers=["Negate"]
)
```

**Natural Language Examples:**
- "Add IA_Jump to IMC_Default mapped to SpaceBar with Pressed trigger"
- "Map the W key to IA_Move in IMC_Default"
- "Bind LeftCtrl to IA_Crouch with a Hold trigger"

**Available Trigger Types:**
- `Down`: Triggered when key is pressed (default behavior)
- `Pressed`: Triggered on key press (one-time event)
- `Released`: Triggered when key is released
- `Hold`: Triggered when key is held for duration
- `HoldAndRelease`: Triggered when held key is released

**Available Modifiers:**
- `Negate`: Inverts the input value (useful for backward movement)
- `SwizzleYXZ`: Swaps axis order
- `DeadZone`: Applies dead zone to analog input

---

## Legacy Input System Tools

### `create_input_mapping`

Creates a legacy input mapping (Action or Axis) in the project settings.

**Parameters:**
- `action_name` (str): Name of the input action
- `key` (str): Key to bind (e.g., "SpaceBar", "LeftMouseButton")
- `input_type` (str, optional): Type of input mapping - "Action" or "Axis" (default: "Action")

**Returns:**
- Dictionary containing the action name and key

**Example:**
```python
# Create a legacy action mapping
result = create_input_mapping(
    action_name="Jump",
    key="SpaceBar",
    input_type="Action"
)
```

**Natural Language Examples:**
- "Create a legacy input action for Jump bound to SpaceBar"
- "Add an axis mapping for MoveForward"

---

## Workflow Examples

### Complete Enhanced Input Setup

```python
# 1. Create input actions
create_input_action(action_name="IA_Jump", value_type="Digital")
create_input_action(action_name="IA_Move", value_type="Axis2D")
create_input_action(action_name="IA_Look", value_type="Axis2D")

# 2. Create mapping context
create_input_mapping_context(context_name="IMC_Default")

# 3. Map jump to space bar
add_action_to_mapping_context(
    context_name="IMC_Default",
    action_name="IA_Jump",
    key="SpaceBar",
    trigger_type="Pressed"
)

# 4. Map movement to WASD
add_action_to_mapping_context(
    context_name="IMC_Default",
    action_name="IA_Move",
    key="W",
    trigger_type="Down"
)

add_action_to_mapping_context(
    context_name="IMC_Default",
    action_name="IA_Move",
    key="S",
    trigger_type="Down",
    modifiers=["Negate"]
)

# 5. Map camera look to mouse
add_action_to_mapping_context(
    context_name="IMC_Default",
    action_name="IA_Look",
    key="MouseX",
    trigger_type="Down"
)

add_action_to_mapping_context(
    context_name="IMC_Default",
    action_name="IA_Look",
    key="MouseY",
    trigger_type="Down"
)
```

### Natural Language Workflow

You can achieve the same setup using natural language:

```
"Create the following input actions: IA_Jump (Digital), IA_Move (Axis2D), and IA_Look (Axis2D)"

"Create an input mapping context named IMC_Default"

"Map IA_Jump to SpaceBar with Pressed trigger in IMC_Default"

"Map W and S keys to IA_Move for forward and backward movement"

"Map mouse X and Y to IA_Look for camera control"
```

---

## Common Key Names

### Keyboard Keys:
- Letter keys: `A`, `B`, `C`, ..., `Z`
- Number keys: `One`, `Two`, `Three`, ..., `Nine`, `Zero`
- Special keys: `SpaceBar`, `Enter`, `Escape`, `Tab`, `BackSpace`
- Modifier keys: `LeftShift`, `RightShift`, `LeftCtrl`, `RightCtrl`, `LeftAlt`, `RightAlt`
- Function keys: `F1`, `F2`, ..., `F12`
- Arrow keys: `Up`, `Down`, `Left`, `Right`

### Mouse:
- Buttons: `LeftMouseButton`, `RightMouseButton`, `MiddleMouseButton`, `ThumbMouseButton`, `ThumbMouseButton2`
- Axes: `MouseX`, `MouseY`, `MouseWheelAxis`

### Gamepad:
- Face buttons: `Gamepad_FaceButton_Bottom`, `Gamepad_FaceButton_Right`, `Gamepad_FaceButton_Left`, `Gamepad_FaceButton_Top`
- Shoulder buttons: `Gamepad_LeftShoulder`, `Gamepad_RightShoulder`
- Triggers: `Gamepad_LeftTrigger`, `Gamepad_RightTrigger`
- Thumbsticks: `Gamepad_LeftX`, `Gamepad_LeftY`, `Gamepad_RightX`, `Gamepad_RightY`
- D-Pad: `Gamepad_DPad_Up`, `Gamepad_DPad_Down`, `Gamepad_DPad_Left`, `Gamepad_DPad_Right`

---

## Tips

1. **Enhanced Input vs Legacy**: Use Enhanced Input System for new projects as it provides more flexibility with triggers and modifiers.

2. **Input Action Naming**: Follow Unreal's naming convention with prefix "IA_" for Input Actions and "IMC_" for Input Mapping Contexts.

3. **Trigger Types**: Choose appropriate trigger types based on the action:
   - `Pressed` for one-time actions (jump, shoot)
   - `Hold` for continuous actions (crouch, aim)
   - `Down` for continuous input (movement, camera)

4. **Modifiers**: Use modifiers to handle complex input transformations without Blueprint logic:
   - `Negate` for opposite directions
   - `DeadZone` for analog stick precision
   - `SwizzleYXZ` for axis remapping

5. **Organization**: Keep all input-related assets in a dedicated folder (e.g., `/Game/Input`) for better project organization.
