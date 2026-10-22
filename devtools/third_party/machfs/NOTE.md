# machfs

Reads and writes Apple HFS volumes in pure Python. devtools/import_originals.py uses it to read the Mac CD image; it is
imported from here (the importers put this folder on sys.path), so nothing needs to be installed.

- Origin: https://github.com/elliotnunn/machfs (the project's PyPI release, https://pypi.org/project/machfs/1.3/)
- Version: 1.3
- License: MIT, Copyright 2018 Elliot Nunn. The license text is LICENSE, copied from the project's repository
  (https://raw.githubusercontent.com/elliotnunn/machfs/master/LICENSE, SHA-256
  1df882042cb67c08b4d8bb3ed48f4f4b62f403f3c3e7dee2151a4dd538513dcf). The release archive carries no license file, and
  its setup.py names the MIT license.
- Downloaded: 2026-10-21 from
  https://files.pythonhosted.org/packages/7f/5e/04d568847e2632f18908636f4e688835292643f5c5cdf93fb764323a6940/machfs-1.3.tar.gz
  (18949 bytes)
- SHA-256 of the archive: 89201818ca16c2385be234b5bfe90946cf80e8141dfa4ed42a54dfa164690389, the digest PyPI publishes
  for that file (https://pypi.org/pypi/machfs/1.3/json); the download matched it.
- Files kept: the package folder machfs/ (`__init__.py`, `bitmanip.py`, `btree.py`, `directory.py`, `main.py`) and
  LICENSE. Dropped: setup.py, the packaging metadata and the README.
- Dependency: macresources (see ../macresources), also vendored here.
- Local changes: none.
