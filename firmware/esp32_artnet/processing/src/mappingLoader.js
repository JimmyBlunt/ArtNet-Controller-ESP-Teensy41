const fs = require("fs");

function validateMapping(mapping) {
  const errors = [];
  if (!mapping || typeof mapping !== "object") errors.push("mapping must be an object");
  if (errors.length) return errors;
  if (!Number.isInteger(mapping.pixelCount) || mapping.pixelCount <= 0) errors.push("pixelCount must be positive");
  if (![1, 2, 3].includes(mapping.dimensions)) errors.push("dimensions must be 1, 2, or 3");
  if (!Array.isArray(mapping.points)) errors.push("points must be an array");
  if (Array.isArray(mapping.points) && mapping.points.length !== mapping.pixelCount) {
    errors.push("points length must equal pixelCount");
  }
  (mapping.points || []).forEach((point, index) => {
    if (!Array.isArray(point) || point.length < mapping.dimensions) {
      errors.push(`point ${index} misses required dimension`);
      return;
    }
    point.slice(0, mapping.dimensions).forEach(value => {
      if (!Number.isFinite(value)) errors.push(`point ${index} has invalid coordinate`);
    });
  });
  return errors;
}

function normalize(points, mode = "contain") {
  if (mode === "none" || points.length === 0) return points.map(point => [...point]);
  const dims = Math.max(...points.map(point => point.length));
  const mins = Array.from({ length: dims }, (_, dim) => Math.min(...points.map(point => point[dim] || 0)));
  const maxs = Array.from({ length: dims }, (_, dim) => Math.max(...points.map(point => point[dim] || 0)));
  const spans = maxs.map((max, dim) => Math.max(1e-9, max - mins[dim]));
  const scale = mode === "contain" ? Math.max(...spans) : null;
  return points.map(point => point.map((value, dim) => {
    const divisor = mode === "contain" ? scale : spans[dim];
    return (value - mins[dim]) / divisor;
  }));
}

function loadMapping(filePath) {
  const mapping = JSON.parse(fs.readFileSync(filePath, "utf8"));
  const errors = validateMapping(mapping);
  if (errors.length) throw new Error(errors.join("; "));
  return {
    ...mapping,
    points: normalize(mapping.points, mapping.fit || "contain")
  };
}

module.exports = { validateMapping, normalize, loadMapping };
