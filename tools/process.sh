cp ../input/doom2d.wad ./processed.wad
python3 ./add_unaligned_skies.py ./processed.wad ./processed.wad
python3 ./trimwad.py ./processed.wad ./processed.wad
python3 ./wad_gfx_replace.py ./processed.wad ./processed.wad
python3 ./align_wad.py ./processed.wad ./processed.wad
cp ./processed.wad ../data/doom2d.wad
