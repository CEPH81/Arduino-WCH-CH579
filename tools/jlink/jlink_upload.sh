#!/bin/sh
# ============================================================================
#  jlink_upload.sh - Carga un .hex en el CH579 con SEGGER J-Link (Linux/macOS)
#  Uso: jlink_upload.sh <carpeta_tool> <archivo.hex> <dispositivo> <kHz>
#  JLinkExe se busca en JLINK_PATH, en el PATH y en las rutas por defecto.
# ============================================================================
TOOLDIR="$1"; HEXFILE="$2"; DEVICE="${3:-CH579M}"; SPEED="${4:-4000}"

[ -f "$HEXFILE" ] || { echo "[CH579] No se encuentra $HEXFILE"; exit 1; }

case "$(uname -s)" in
  Darwin) DEVROOT="$HOME/Library/Application Support/SEGGER/JLinkDevices" ;;
  *)      DEVROOT="$HOME/.config/SEGGER/JLinkDevices" ;;
esac
DEVDIR="$DEVROOT/WCH/$DEVICE"
mkdir -p "$DEVDIR"
cp -f "$TOOLDIR/CH579M/JLinkDevices.xml" "$TOOLDIR/CH579M/CH57x.FLM" "$DEVDIR/"

JLINK=""
if [ -n "$JLINK_PATH" ] && [ -x "$JLINK_PATH/JLinkExe" ]; then JLINK="$JLINK_PATH/JLinkExe"; fi
[ -z "$JLINK" ] && JLINK="$(command -v JLinkExe 2>/dev/null)"
for c in /opt/SEGGER/JLink/JLinkExe /Applications/SEGGER/JLink/JLinkExe /usr/bin/JLinkExe; do
  [ -z "$JLINK" ] && [ -x "$c" ] && JLINK="$c"
done
[ -n "$JLINK" ] || { echo "[CH579] No se encontro JLinkExe (instale el J-Link Software Pack o defina JLINK_PATH)"; exit 1; }

SCRIPT="$(mktemp /tmp/ch579_jlink.XXXXXX)"
printf 'r\nh\nloadfile "%s"\nr\ng\nexit\n' "$HEXFILE" > "$SCRIPT"
echo "[CH579] J-Link: $JLINK  -  $DEVICE, SWD a $SPEED kHz"
"$JLINK" -device "$DEVICE" -if SWD -speed "$SPEED" -autoconnect 1 -NoGui 1 -ExitOnError 1 -CommanderScript "$SCRIPT"
RC=$?
rm -f "$SCRIPT"
[ $RC -eq 0 ] && echo "[CH579] Carga completada." || echo "[CH579] La carga fallo (codigo $RC). Revise SWDIO=PB16, SWCLK=PB17, GND, VTref."
exit $RC
