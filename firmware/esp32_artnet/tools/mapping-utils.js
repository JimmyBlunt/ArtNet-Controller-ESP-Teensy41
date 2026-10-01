const fs = require("fs");

function isFinitePoint(point) {
  return Array.isArray(point) && point.length >= 1 && point.length <= 3 &&
    point.every(Number.isFinite);
}

function stripJsonComments(text) {
  let out = "";
  let inString = false;
  let quote = "";
  let escaped = false;
  let inLineComment = false;
  let inBlockComment = false;

  for (let i = 0; i < text.length; i += 1) {
    const ch = text[i];
    const next = text[i + 1];

    if (inLineComment) {
      if (ch === "\n" || ch === "\r") {
        inLineComment = false;
        out += ch;
      }
      continue;
    }

    if (inBlockComment) {
      if (ch === "*" && next === "/") {
        inBlockComment = false;
        i += 1;
      } else if (ch === "\n" || ch === "\r") {
        out += ch;
      }
      continue;
    }

    if (inString) {
      out += ch;
      if (escaped) {
        escaped = false;
      } else if (ch === "\\") {
        escaped = true;
      } else if (ch === quote) {
        inString = false;
      }
      continue;
    }

    if (ch === "\"" || ch === "'") {
      inString = true;
      quote = ch;
      out += ch;
      continue;
    }

    if (ch === "/" && next === "/") {
      inLineComment = true;
      i += 1;
      continue;
    }

    if (ch === "/" && next === "*") {
      inBlockComment = true;
      i += 1;
      continue;
    }

    out += ch;
  }

  return out;
}

function removeTrailingCommas(text) {
  let out = "";
  let inString = false;
  let quote = "";
  let escaped = false;

  for (let i = 0; i < text.length; i += 1) {
    const ch = text[i];
    if (inString) {
      out += ch;
      if (escaped) {
        escaped = false;
      } else if (ch === "\\") {
        escaped = true;
      } else if (ch === quote) {
        inString = false;
      }
      continue;
    }

    if (ch === "\"" || ch === "'") {
      inString = true;
      quote = ch;
      out += ch;
      continue;
    }

    if (ch === ",") {
      let j = i + 1;
      while (j < text.length && /\s/.test(text[j])) j += 1;
      if (text[j] === "]" || text[j] === "}") continue;
    }

    out += ch;
  }

  return out;
}

function validateMapping(mapping) {
  if (!mapping || typeof mapping !== "object") return ["mapping must be an object"];
  const errors = [];
  if (!mapping.id) errors.push("id is required");
  if (mapping.version !== 1) errors.push("version must be 1");
  if (!Number.isInteger(mapping.pixelCount) || mapping.pixelCount < 1) errors.push("pixelCount must be >= 1");
  if (![1, 2, 3].includes(mapping.dimensions)) errors.push("dimensions must be 1, 2 or 3");
  if (!Array.isArray(mapping.points)) errors.push("points must be an array");
  if (Array.isArray(mapping.points) && Number.isInteger(mapping.pixelCount) && mapping.points.length !== mapping.pixelCount) {
    errors.push("points length must equal pixelCount");
  }
  if (Array.isArray(mapping.points)) {
    mapping.points.forEach((point, index) => {
      if (!isFinitePoint(point)) errors.push(`point ${index} contains invalid coordinates`);
      if (Array.isArray(point) && point.length < mapping.dimensions) errors.push(`point ${index} misses required dimension`);
    });
  }
  return errors;
}

function normalize(points, mode) {
  const dims = [0, 1, 2].map(axis => points.map(p => p[axis] || 0));
  const min = dims.map(values => Math.min(...values));
  const max = dims.map(values => Math.max(...values));
  const spans = max.map((v, i) => Math.max(1e-9, v - min[i]));
  const scale = mode === "fill" ? Math.min(...spans) : Math.max(...spans);
  return points.map(p => [0, 1, 2].map(axis => ((p[axis] || 0) - min[axis]) / scale));
}

function pointsToMapping(points, id, source, fit = "contain") {
  if (!Array.isArray(points)) throw new Error("input must be a JSON array of points");
  points.forEach((point, index) => {
    if (!isFinitePoint(point)) throw new Error(`invalid point at index ${index}`);
  });
  const dimensions = points.reduce((max, point) => Math.max(max, point.length), 1);
  return { id, version: 1, pixelCount: points.length, dimensions, fit, source, points };
}

function looksLikePixelblazeGenerator(text) {
  return /\bmapPixels\s*\(/.test(text) ||
    /\bexport\s+function\s+(render|render2D|render3D|beforeRender)\b/.test(text) ||
    /\bfunction\s+(render|render2D|render3D|beforeRender)\b/.test(text);
}

function importPixelblaze(text, id = "pixelblaze-import") {
  const cleaned = removeTrailingCommas(stripJsonComments(text));
  try {
    const parsed = JSON.parse(cleaned);
    if (!Array.isArray(parsed)) throw new Error("Pixelblaze mapper JSON must be a coordinate array");
    return {
      kind: "mapping",
      mapping: pointsToMapping(parsed, id, "pixelblaze-array", "contain")
    };
  } catch (error) {
    if (!looksLikePixelblazeGenerator(text)) throw error;
    return {
      kind: "generator",
      generator: {
        id,
        version: 1,
        source: "pixelblaze-js-generator",
        executable: false,
        generatorSource: text,
        warnings: [
          "Pixelblaze JavaScript generators are preserved for offline review and are not executed by this tool or on the ESP32."
        ]
      }
    };
  }
}

function importPixelblazeArray(text, id = "pixelblaze-import") {
  const result = importPixelblaze(text, id);
  if (result.kind !== "mapping") {
    throw new Error("Pixelblaze JavaScript mapper generators cannot be converted without running the original Pixelblaze environment");
  }
  return result.mapping;
}

function parseCsv(text) {
  const rows = [];
  let row = [];
  let cell = "";
  let inQuotes = false;

  for (let i = 0; i < text.length; i += 1) {
    const ch = text[i];
    const next = text[i + 1];

    if (inQuotes) {
      if (ch === "\"" && next === "\"") {
        cell += "\"";
        i += 1;
      } else if (ch === "\"") {
        inQuotes = false;
      } else {
        cell += ch;
      }
      continue;
    }

    if (ch === "\"") {
      inQuotes = true;
    } else if (ch === ",") {
      row.push(cell);
      cell = "";
    } else if (ch === "\n") {
      row.push(cell);
      rows.push(row);
      row = [];
      cell = "";
    } else if (ch !== "\r") {
      cell += ch;
    }
  }

  if (cell.length || row.length) {
    row.push(cell);
    rows.push(row);
  }

  return rows.filter(cells => cells.some(value => String(value).trim() !== ""));
}

function numberFromCsv(value, label) {
  const parsed = Number(String(value).trim());
  if (!Number.isFinite(parsed)) throw new Error(`invalid numeric value for ${label}: ${value}`);
  return parsed;
}

function importMariMapperCsv(text, id = "marimapper-import") {
  const rows = parseCsv(text);
  if (rows.length < 2) throw new Error("MariMapper CSV needs a header and at least one point row");

  const headers = rows[0].map(header => String(header).trim());
  const lowerHeaders = headers.map(header => header.toLowerCase());
  const column = name => lowerHeaders.indexOf(name);
  const indexCol = column("index");
  const xCol = column("x") !== -1 ? column("x") : column("tx");
  const yCol = column("y") !== -1 ? column("y") : column("ty");
  const zCol = column("z") !== -1 ? column("z") : column("tz");
  if (indexCol === -1 || xCol === -1 || yCol === -1) {
    throw new Error("MariMapper CSV needs columns: index plus x/y/z or tx/ty/tz");
  }

  const coordCols = new Set([indexCol, xCol, yCol]);
  if (zCol !== -1) coordCols.add(zCol);
  const metadataFields = headers.filter((_, index) => !coordCols.has(index));
  const pointsByIndex = new Map();
  const metadataByIndex = new Map();

  rows.slice(1).forEach((cells, rowIndex) => {
    const index = Number.parseInt(String(cells[indexCol] || "").trim(), 10);
    if (!Number.isInteger(index) || index < 0) throw new Error(`invalid index at CSV row ${rowIndex + 2}`);
    if (pointsByIndex.has(index)) throw new Error(`duplicate index ${index} at CSV row ${rowIndex + 2}`);
    const point = [
      numberFromCsv(cells[xCol], `x at CSV row ${rowIndex + 2}`),
      numberFromCsv(cells[yCol], `y at CSV row ${rowIndex + 2}`)
    ];
    if (zCol !== -1) point.push(numberFromCsv(cells[zCol], `z at CSV row ${rowIndex + 2}`));
    pointsByIndex.set(index, point);

    if (metadataFields.length) {
      const metadata = {};
      headers.forEach((header, colIndex) => {
        if (!coordCols.has(colIndex)) metadata[header] = cells[colIndex] || "";
      });
      metadataByIndex.set(index, metadata);
    }
  });

  const maxIndex = Math.max(...pointsByIndex.keys());
  const points = [];
  const missingIndices = [];
  const rowMetadata = [];
  const dimensions = zCol === -1 ? 2 : 3;
  for (let index = 0; index <= maxIndex; index += 1) {
    if (pointsByIndex.has(index)) {
      points.push(pointsByIndex.get(index));
      if (metadataFields.length) rowMetadata.push(metadataByIndex.get(index) || {});
    } else {
      missingIndices.push(index);
      points.push(Array(dimensions).fill(0));
      if (metadataFields.length) rowMetadata.push({});
    }
  }

  const warnings = missingIndices.length
    ? [`Filled ${missingIndices.length} missing MariMapper index slot(s) with [0,0${dimensions === 3 ? ",0" : ""}] placeholders.`]
    : [];

  return {
    id,
    version: 1,
    pixelCount: points.length,
    dimensions,
    fit: "none",
    source: "marimapper-csv",
    points,
    compatibility: {
      sourceFormat: "marimapper-csv",
      missingIndices,
      metadataFields,
      warnings
    },
    ...(metadataFields.length ? { rowMetadata } : {})
  };
}

function exportPixelblazeArray(mapping, options = {}) {
  const errors = validateMapping(mapping);
  if (errors.length) throw new Error(`mapping is invalid:\n${errors.join("\n")}`);
  return `${JSON.stringify(mapping.points, null, options.pretty ? 2 : 0)}\n`;
}

function csvEscape(value) {
  const stringValue = String(value == null ? "" : value);
  if (/[",\n\r]/.test(stringValue)) return `"${stringValue.replace(/"/g, "\"\"")}"`;
  return stringValue;
}

function exportMariMapperCsv(mapping) {
  const errors = validateMapping(mapping);
  if (errors.length) throw new Error(`mapping is invalid:\n${errors.join("\n")}`);
  const metadataFields = mapping.compatibility && mapping.compatibility.metadataFields
    ? mapping.compatibility.metadataFields
    : [];
  const rows = [
    ["index", "x", "y", ...(mapping.dimensions >= 3 ? ["z"] : []), ...metadataFields]
  ];
  mapping.points.forEach((point, index) => {
    const metadata = mapping.rowMetadata && mapping.rowMetadata[index] ? mapping.rowMetadata[index] : {};
    rows.push([
      index,
      point[0] || 0,
      point[1] || 0,
      ...(mapping.dimensions >= 3 ? [point[2] || 0] : []),
      ...metadataFields.map(field => metadata[field] || "")
    ]);
  });
  return `${rows.map(row => row.map(csvEscape).join(",")).join("\n")}\n`;
}

function loadMapping(path) {
  return JSON.parse(fs.readFileSync(path, "utf8"));
}

module.exports = {
  validateMapping,
  normalize,
  importPixelblaze,
  importPixelblazeArray,
  importMariMapperCsv,
  exportPixelblazeArray,
  exportMariMapperCsv,
  loadMapping,
  stripJsonComments,
  removeTrailingCommas
};
