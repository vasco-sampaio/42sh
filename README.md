# Building

```sh
meson setup builddir
meson compile -C builddir  # or ninja -C builddir
builddir/42sh
```

# Debugging

```
gdb -arg builddir/42sh -c 'echo test'
```

# Running tests

```sh
cd
python -m venv env/
source env/bin/activate
go to 42sh folder
pip install -r tests/requirements.txt
```

# Building documentation

```sh
# enable documentation support
meson setup -Ddoc=true builddir  # --reconfigure might be needed
# build the documentation
meson compile -C builddir doxygen_doc
# open the documentation in the browser
xdg-open builddir/doxygen_doc/index.html
```

