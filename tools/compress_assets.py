# compress_assets.py
import gzip
import os


def compress_file(input_path, output_path):
    with open(input_path, "rb") as f:
        content = f.read()

    compressed = gzip.compress(content, 9)  # Maximum compression

    with open(output_path, "wb") as f:
        f.write(compressed)

    return len(content), len(compressed)


def generate_header_file(input_dir, output_file):
    files_to_compress = [
        ("index.html", "text/html"),
        ("styles.css", "text/css"),
        ("script.js", "application/javascript"),
        ("wifi_portal.html", "text/html"),
        ("wifi_portal.css", "text/css"),
        ("wifi_portal.js", "application/javascript"),
    ]

    # Start writing the header file
    with open(output_file, "w") as header:
        header.write("#ifndef GZIPPED_ASSETS_H\n")
        header.write("#define GZIPPED_ASSETS_H\n\n")
        header.write("#include <Arduino.h>\n\n")

        # Create a struct for asset information
        header.write("struct GzipAsset {\n")
        header.write("  const char* path;\n")
        header.write("  const char* content_type;\n")
        header.write("  const uint8_t* data;\n")
        header.write("  size_t length;\n")
        header.write("};\n\n")

        # Process each file
        for filename, content_type in files_to_compress:
            input_path = os.path.join(input_dir, filename)
            gzip_path = f"{input_path}.gz"

            # Skip if file doesn't exist
            if not os.path.exists(input_path):
                print(f"Warning: {input_path} not found, skipping")
                continue

            # Compress the file
            original_size, compressed_size = compress_file(input_path,
                                                           gzip_path)

            # Read the compressed content
            with open(gzip_path, "rb") as f:
                compressed_content = f.read()

            # Convert to hex array
            name_without_dots = filename.replace(".", "_")
            var_name = f"gzip_{name_without_dots.replace('-', '_')}"

            header.write(f"// {filename}: ")
            header.write(f"{original_size} bytes -> {compressed_size} bytes ")
            header.write(f"({compressed_size/original_size*100:.1f}%)\n")
            header.write(f"static const uint8_t {var_name}[] PROGMEM = {{\n  ")

            # Write byte values in hex format with line breaks
            byte_count = 0
            for byte in compressed_content:
                header.write(f"0x{byte:02x}, ")
                byte_count += 1
                if byte_count % 12 == 0:  # 12 bytes per line for readability
                    header.write("\n  ")

            header.write("\n};\n\n")

            # Clean up temp file
            os.remove(gzip_path)

        # Create asset table
        header.write("static const GzipAsset GZIP_ASSETS[] = {\n")
        for filename, content_type in files_to_compress:
            if os.path.exists(os.path.join(input_dir, filename)):
                name_without_dots = filename.replace(".", "_")
                var_name = f"gzip_{name_without_dots.replace('-', '_')}"

                # Use the correct path for serving
                if filename == "index.html":
                    path = "/"  # Serve index.html at root path
                else:
                    path = f"/{filename}"

                # Split the asset entry into multiple parts
                asset_entry = f'  {{ "{path}", "{content_type}", '
                asset_entry += f"{var_name}, sizeof({var_name}) }},\n"
                header.write(asset_entry)
        header.write("};\n\n")

        # Write asset count
        header.write("static const int GZIP_ASSETS_COUNT = ")
        header.write("sizeof(GZIP_ASSETS) / sizeof(GzipAsset);\n\n")
        header.write("#endif // GZIPPED_ASSETS_H\n")

    print(f"Generated {output_file}")


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
    data_dir = os.path.join(project_root, "dashboard")
    output_file = os.path.join(project_root, "gzipped_assets.h")

    print(f"Project root: {project_root}")
    print(f"Data directory: {data_dir}")
    print(f"Output file: {output_file}")

    generate_header_file(data_dir, output_file)
