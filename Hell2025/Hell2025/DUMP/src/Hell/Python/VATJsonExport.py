import json
import re
from pathlib import Path

try:
    import hou
except ImportError:
    hou = None


# Edit the name for each VAT export. The script expects <name>_mat.mat.
name = "Blood8_optimized"

material_directory = Path(r"C:/Users/User/Desktop/Blood/Houdini/Exports")
output_directory = Path(r"C:/Hell2025/Hell2025/Hell2025/res/VAT")


NUMBER_PATTERN = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?"


def read_material_float(material_text: str, property_name: str) -> float:
    pattern = rf"^\s*-\s+{re.escape(property_name)}:\s*({NUMBER_PATTERN})\s*$"
    match = re.search(pattern, material_text, flags=re.MULTILINE)

    if match is None:
        raise ValueError(f"Unity material is missing property: {property_name}")

    return float(match.group(1))


def read_vat_metadata(material_path: Path):
    material_text = material_path.read_text(encoding="utf-8-sig")

    frame_count_value = read_material_float(material_text, "_frameCount")
    frame_count = int(round(frame_count_value))
    fps = read_material_float(material_text, "_houdiniFPS")

    bounds_min = [
        read_material_float(material_text, "_boundMinX"),
        read_material_float(material_text, "_boundMinY"),
        read_material_float(material_text, "_boundMinZ"),
    ]
    bounds_max = [
        read_material_float(material_text, "_boundMaxX"),
        read_material_float(material_text, "_boundMaxY"),
        read_material_float(material_text, "_boundMaxZ"),
    ]

    if frame_count <= 0:
        raise ValueError(f"Invalid _frameCount value: {frame_count_value}")
    if fps <= 0.0:
        raise ValueError(f"Invalid _houdiniFPS value: {fps}")

    for axis, (minimum, maximum) in enumerate(zip(bounds_min, bounds_max)):
        if minimum > maximum:
            raise ValueError(
                f"Invalid bounds on axis {axis}: minimum {minimum} exceeds maximum {maximum}"
            )

    return frame_count, fps, bounds_min, bounds_max


def export_vat_json() -> Path:
    if not name or Path(name).name != name:
        raise ValueError("name must be a non-empty filename without a path")

    material_path = material_directory / f"{name}_mat.mat"
    if not material_path.is_file():
        raise FileNotFoundError(f"Could not find Unity VAT material: {material_path}")

    frame_count, fps, bounds_min, bounds_max = read_vat_metadata(material_path)

    metadata = {
        "frameCount": frame_count,
        "fps": fps,
        "boundsMin": bounds_min,
        "boundsMax": bounds_max,
        "positionTexture": f"{name}_pos.exr",
        "rotationTexture": f"{name}_rot.exr",
        "lookupTexture": f"{name}_lookup.png",
        "model": f"{name}_mesh.fbx",
    }

    output_directory.mkdir(parents=True, exist_ok=True)
    output_path = output_directory / f"{name}.json"
    output_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")

    print(f"Read Unity VAT material: {material_path}")
    print(f"Wrote VAT metadata: {output_path}")
    print(f"Frame count: {frame_count}")
    print(f"FPS: {fps}")
    print(f"Bounds min: {bounds_min}")
    print(f"Bounds max: {bounds_max}")
    return output_path


def show_message(message: str, is_error: bool = False):
    if hou is not None and hou.isUIAvailable():
        severity = hou.severityType.Error if is_error else hou.severityType.Message
        hou.ui.displayMessage(
            message,
            severity=severity,
            title="VAT JSON Export",
        )
    else:
        print(message)


def run_export():
    try:
        output_path = export_vat_json()
    except Exception as exc:
        show_message(f"VAT JSON export failed:\n\n{exc}", is_error=True)
        raise

    show_message(f"VAT JSON export complete.\n\n{output_path}")
    return output_path


# Execute when run from Houdini's Python UI or as a normal Python script.
run_export()
