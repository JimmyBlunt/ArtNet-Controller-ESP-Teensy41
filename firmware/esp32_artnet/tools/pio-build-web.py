"""Regenerate the shared UI before PlatformIO scans firmware dependencies."""
Import("env")
import os
import subprocess

subprocess.check_call(["node", os.path.join(env.subst("$PROJECT_DIR"), "tools", "build-web-ui.js")])
