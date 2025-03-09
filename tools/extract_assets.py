# extract_assets.py
import re
import os
import gzip


def extract_assets_from_header(header_file, output_dir):
    """Extract gzipped assets from a C++ header file back to their original"""
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Check if header file exists
    if not os.path.exists(header_file):
        print(f"Error: Header file '{header_file}' does not exist!")
        return False

    with open(header_file, "r") as f:
        content = f.read()

    # Print header file info for debugging
    print(f"Header file size: {len(content)} bytes")

    # Try to find the asset table with a more flexible pattern
    asset_pattern = (
        r"static\s+const\s+GzipAsset\s+GZIP_ASSETS\[\]\s*=\s*\{(.*?)\}\s*;"
    )
    assets_match = re.search(asset_pattern, content, re.DOTALL)

    if not assets_match:
        print("Could not find asset table. Looking for individual assets...")

        # Find any gzipped arrays directly
        array_defs = re.findall(
            r"static const uint8_t (gzip_[^\[]+)\[\] PROGMEM = \{([^}]+)\};",
            content,
            re.DOTALL,
        )

        if not array_defs:
            print("No gzipped assets found in the header file.")
            return False

        print(f"Found {len(array_defs)} potential assets. Trying to extract..")

        for var_name, hex_data in array_defs:
            try:
                # Try to guess filename from variable name
                if "index_html" in var_name:
                    file_name = "index.html"
                elif "styles_css" in var_name:
                    file_name = "styles.css"
                elif "script_js" in var_name:
                    file_name = "script.js"
                elif "wifi_portal_html" in var_name:
                    file_name = "wifi_portal.html"
                elif "wifi_portal_css" in var_name:
                    file_name = "wifi_portal.css"
                elif "wifi_portal_js" in var_name:
                    file_name = "wifi_portal.js"
                elif "ei_config_json" in var_name:
                    file_name = "ei_config.json"
                elif "ei_config_template_json" in var_name:
                    file_name = "ei_config.template.json"
                else:
                    # Create a generic name based on the variable
                    file_name = var_name.replace("gzip_", "").replace("_", ".")

                # Extract the binary data
                hex_values = re.findall(r"0x([0-9a-fA-F]{2})", hex_data)
                binary_data = bytearray([int(x, 16) for x in hex_values])

                # Try to decompress
                decompressed = gzip.decompress(binary_data)

                # Save the file
                output_path = os.path.join(output_dir, file_name)
                os.makedirs(os.path.dirname(output_path), exist_ok=True)

                with open(output_path, "wb") as f:
                    f.write(decompressed)

                print(f"Extracted {file_name} ({len(decompressed)} bytes)")

            except Exception as e:
                print(f"Error extracting {var_name}: {e}")

        return True

    # If we found the regular asset table, use it
    asset_table = assets_match.group(1)

    # Extract each asset record
    asset_entries = re.findall(
        r'\{\s*"([^"]+)"\s*,\s*"[^"]+"\s*,\s*([^,]+)\s*,', asset_table
    )

    # Extract each binary array
    for path, array_name in asset_entries:
        # Clean up the path (remove leading slash and handle root path)
        if path == "/":
            file_name = "index.html"  # Root path is index.html
        else:
            file_name = path.lstrip("/")

        # Find the array data
        array_pattern = (
            rf"static const uint8_t {array_name}\[\] PROGMEM = \{{([^}}]+)\}};"
        )
        array_match = re.search(array_pattern, content, re.DOTALL)
        if not array_match:
            print(f"Could not find data array for {file_name}")
            continue

        # Extract hex values
        hex_data = array_match.group(1)
        hex_values = re.findall(r"0x([0-9a-fA-F]{2})", hex_data)
        if not hex_values:
            print(f"No hex values found for {file_name}")
            continue

        # Convert hex to binary
        binary_data = bytearray([int(x, 16) for x in hex_values])

        # Decompress gzip data
        try:
            decompressed = gzip.decompress(binary_data)

            # Write to output file
            output_path = os.path.join(output_dir, file_name)
            os.makedirs(os.path.dirname(output_path), exist_ok=True)

            with open(output_path, "wb") as f:
                f.write(decompressed)

            print(f"Extracted {file_name} ({len(decompressed)} bytes)")
        except Exception as e:
            print(f"Error decompressing {file_name}: {e}")

    return True


if __name__ == "__main__":
    # Get the directory of this script
    script_dir = os.path.dirname(os.path.abspath(__file__))

    # Get the project root directory
    # One level up if script is in a subdirectory
    if os.path.basename(script_dir) == "tools":
        project_root = os.path.dirname(script_dir)
    else:
        project_root = script_dir

    # Define paths relative to the project root
    header_file = os.path.join(project_root, "gzipped_assets.h")
    output_dir = os.path.join(project_root, "web_assets_extracted")

    print(f"Project root: {project_root}")
    print(f"Header file: {header_file}")
    print(f"Output directory: {output_dir}")

    extract_assets_from_header(header_file, output_dir)
    print(f"Extraction complete. Files are in '{output_dir}' directory.")
