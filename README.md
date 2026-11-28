# Ayatana Appindicator Display


### Build dependencies:

- `cmake`
- `cmake-extras`
- `intltool`
- `glib2-devel`

### Runtime dependencies:

- `glibc`
- `glib2`
- `geoclue`
- `libayatana-appindicator-glib`
- `libayatana-common`
- `hicolor-icon-theme`
- `dconf`

### Release build:

```
cmake -S . -B build
cmake --build build
cmake --install build
```


### Test build:

```
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build
cmake --build build
```
