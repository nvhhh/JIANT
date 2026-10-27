# JIANT FM: referencias de la interfaz

Las 20 pantallas del concepto JIANT FM ([docs/jiant-fm-ui-concept.jpg](../../docs/jiant-fm-ui-concept.jpg)),
recortadas a 240 × 240. **Son solo referencia visual**: el firmware dibuja todo por código
(`firmware/src/ui_organic.c`), sin imágenes guardadas, para no gastar flash. Plan en
[docs/TONIC-UI.md](../../docs/TONIC-UI.md).

La excepción es el monje de MONK ([monk/](monk/), seis caras: rest ah oo ee mm chant), guardado como
imagen: `tools/gen_monk.py` lo reduce a 112 × 112 px y ~6,7 KB (`firmware/src/ui_monk.h`).

Las formas que dibuja el firmware están en [assets/ui-shapes/](../ui-shapes/) (SVG).
