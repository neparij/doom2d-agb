#!/bin/sh
set -e

INPUT_WAD=../input/doom2d.wad
OUTPUT_DIR=../audio
DMM2XM=./dmm2xm

# WAD uses legacy encoding (CP866/IBM866); -f for lookup, -o translit lower_case.
to_wad() {
  w=$(printf '%s' "$1" | iconv -f UTF-8 -t CP866 2>/dev/null)
  [ -n "$w" ] || w=$(printf '%s' "$1" | iconv -f UTF-8 -t IBM866 2>/dev/null)
  printf '%s' "${w:-$1}"
}

# dmm_to_xm
# Convert DMM music from WAD to XM format, using dmm2xm tool.
# $1: WAD lump name (utf8)
# $2: output XM name (without path and extension)
# $3: -q value
dmm_to_xm() {
  echo "    Converting $1"
  local wad_name=$(to_wad "$1")
  local xm_name="$2"
  local q_value="$3"

  if output=$("${DMM2XM}" -q "${q_value}" \
              -f "$(to_wad "${wad_name}")" \
              -o "${OUTPUT_DIR}/${xm_name}.xm" \
              "${INPUT_WAD}" 2>&1); then
      if echo "$output" | grep -qi "error"; then
          echo "Error converting ${wad_name} → ${xm_name}.xm:"
          echo "$output"
          exit 1
      fi
  else
      echo "Error converting ${wad_name} → ${xm_name}.xm:"
      echo "$output"
      exit 1
  fi
}

[ -f ${INPUT_WAD} ] || { echo "WAD file is missing."; exit 1; }
echo "Converting DMM music from WAD to XM format..."
dmm_to_xm "ALLRIGHT" allright 8
dmm_to_xm "BEST" best 6
dmm_to_xm "GLAD" glad 8
dmm_to_xm "MENU" menu 22
dmm_to_xm "INTERMUS" intermus 16
dmm_to_xm "SUPER" super 12
dmm_to_xm "АС" as 8
dmm_to_xm "АТАС" atas 12
dmm_to_xm "АЙОЙ" ayoy 8
dmm_to_xm "БОЖ_МУЗ" bozh_muz 8
dmm_to_xm "БЫСТРОТА" bystrota 12
dmm_to_xm "С_ВЫШАК" s_vyshak 8
dmm_to_xm "КЛАСС" klass 8
dmm_to_xm "ДУШЕВНАЯ" dushevnaya 8
dmm_to_xm "ХЕХЕХЕ" hehehe 16
dmm_to_xm "ХОХОХО" hohoho 8
dmm_to_xm "КОНЕЦ" konets 10
dmm_to_xm "МОЩЬ" moshch 8
dmm_to_xm "ПАЛАТА#6" palata_6 8
dmm_to_xm "ПРИКОЛ" prikol 8
dmm_to_xm "ПРОБА" proba 8
dmm_to_xm "ПРОСТОТА" prostota 8
dmm_to_xm "СОЛО" solo 8
dmm_to_xm "СПОК" spok 12
dmm_to_xm "СТРАХ" strah 8
dmm_to_xm "ТИШЬ" tish 12
dmm_to_xm "ТЫ_ПОБЕД" ty_pobed 8
dmm_to_xm "ТЫ_ТРУП" ty_trup 16
dmm_to_xm "УЖАС" uzhas 12
dmm_to_xm "ВЗБУЧКА" vzbuchka 12

echo "Finished."
exit 0
