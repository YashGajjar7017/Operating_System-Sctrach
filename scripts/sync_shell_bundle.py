#!/usr/bin/env python3
"""
Convert Vite Desktop Shell build output into an embedded C header for Xenithra OS
"""

import os
import sys

def embed_dist():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.dirname(script_dir)
    dist_dir = os.path.join(root_dir, "desktop_shell", "dist")
    out_header = os.path.join(root_dir, "kernel", "gui", "react_bundle.h")

    if not os.path.exists(dist_dir):
        print(f"[-] Error: {dist_dir} does not exist. Run 'npm run build' inside desktop_shell first.")
        return False

    index_html_path = os.path.join(dist_dir, "index.html")
    if not os.path.exists(index_html_path):
        print("[-] Error: dist/index.html not found.")
        return False

    with open(index_html_path, "rb") as f:
        html_bytes = f.read()

    with open(out_header, "w", encoding="utf-8") as out:
        out.write("/* Auto-generated from desktop_shell/dist/index.html */\n")
        out.write("#ifndef _KERNEL_GUI_REACT_BUNDLE_H_\n")
        out.write("#define _KERNEL_GUI_REACT_BUNDLE_H_\n\n")
        out.write("#include <stdint.h>\n#include <stddef.h>\n\n")
        out.write(f"static const size_t g_react_bundle_size = {len(html_bytes)};\n")
        out.write("static const uint8_t g_react_bundle_data[] = {\n")
        
        # Write bytes in rows of 16
        for i, b in enumerate(html_bytes):
            out.write(f"0x{b:02x}, ")
            if (i + 1) % 16 == 0:
                out.write("\n")
        
        out.write("\n0x00\n};\n\n")
        out.write("#endif /* _KERNEL_GUI_REACT_BUNDLE_H_ */\n")

    print(f"[+] Successfully embedded React Desktop Shell bundle ({len(html_bytes)} bytes) -> {out_header}")
    return True

if __name__ == "__main__":
    embed_dist()
