const encoder = new TextEncoder();
const LOCAL_SIGNATURE = 0x04034b50;
const CENTRAL_SIGNATURE = 0x02014b50;
const END_SIGNATURE = 0x06054b50;
const DOS_DATE = ((2026 - 1980) << 9) | (1 << 5) | 1;

const CRC_TABLE = Uint32Array.from({ length: 256 }, (_, n) => {
  let value = n;
  for (let bit = 0; bit < 8; bit++) {
    value = value & 1 ? 0xedb88320 ^ (value >>> 1) : value >>> 1;
  }
  return value >>> 0;
});

export function crc32(bytes) {
  let crc = 0xffffffff;
  for (const byte of bytes) {
    crc = CRC_TABLE[(crc ^ byte) & 0xff] ^ (crc >>> 8);
  }
  return (crc ^ 0xffffffff) >>> 0;
}

function record(size) {
  const bytes = new Uint8Array(size);
  return { bytes, view: new DataView(bytes.buffer) };
}

export function createZip(files) {
  const parts = [];
  const central = [];
  let offset = 0;

  for (const file of files) {
    const name = encoder.encode(file.name);
    const data = encoder.encode(file.text);
    const crc = crc32(data);

    const local = record(30 + name.length);
    local.view.setUint32(0, LOCAL_SIGNATURE, true);
    local.view.setUint16(4, 20, true);
    local.view.setUint16(6, 0x0800, true);
    local.view.setUint16(12, DOS_DATE, true);
    local.view.setUint32(14, crc, true);
    local.view.setUint32(18, data.length, true);
    local.view.setUint32(22, data.length, true);
    local.view.setUint16(26, name.length, true);
    local.bytes.set(name, 30);
    parts.push(local.bytes, data);

    const entry = record(46 + name.length);
    entry.view.setUint32(0, CENTRAL_SIGNATURE, true);
    entry.view.setUint16(4, 20, true);
    entry.view.setUint16(6, 20, true);
    entry.view.setUint16(8, 0x0800, true);
    entry.view.setUint16(14, DOS_DATE, true);
    entry.view.setUint32(16, crc, true);
    entry.view.setUint32(20, data.length, true);
    entry.view.setUint32(24, data.length, true);
    entry.view.setUint16(28, name.length, true);
    entry.view.setUint32(42, offset, true);
    entry.bytes.set(name, 46);
    central.push(entry.bytes);

    offset += local.bytes.length + data.length;
  }

  const directorySize = central.reduce((total, entry) => total + entry.length, 0);
  const end = record(22);
  end.view.setUint32(0, END_SIGNATURE, true);
  end.view.setUint16(8, files.length, true);
  end.view.setUint16(10, files.length, true);
  end.view.setUint32(12, directorySize, true);
  end.view.setUint32(16, offset, true);

  return new Blob([...parts, ...central, end.bytes], { type: "application/zip" });
}
