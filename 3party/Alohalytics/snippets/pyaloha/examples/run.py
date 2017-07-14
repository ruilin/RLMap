import os
import sys

base_path = os.path.dirname(os.path.abspath(__file__))

pyaloha_path = os.path.join(
    base_path, '..', '..'
)

sys.path.append(pyaloha_path)

PYSCRIPT_PATH = base_path

pyaloha = __import__('pyaloha.main')
pyaloha.main.cmd_run(plugin_dir=PYSCRIPT_PATH)
