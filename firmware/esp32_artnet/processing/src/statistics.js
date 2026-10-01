class VisualizerStats {
  constructor({ now = () => Date.now() } = {}) {
    this.now = now;
    this.receivedFrames = 0;
    this.renderedFrames = 0;
    this.lostFrames = 0;
    this.incompleteFrames = 0;
    this.bytes = 0;
    this.lastFrameId = null;
    this.startedAt = this.now();
  }

  recordPacket(byteLength) {
    this.bytes += byteLength;
  }

  recordFrame(frameId) {
    if (this.lastFrameId !== null && frameId > this.lastFrameId + 1) {
      this.lostFrames += frameId - this.lastFrameId - 1;
    }
    this.lastFrameId = frameId;
    this.receivedFrames++;
  }

  recordIncomplete() {
    this.incompleteFrames++;
  }

  recordRendered() {
    this.renderedFrames++;
  }

  snapshot() {
    const seconds = Math.max(0.001, (this.now() - this.startedAt) / 1000);
    return {
      receivedFps: this.receivedFrames / seconds,
      renderedFps: this.renderedFrames / seconds,
      lostFrames: this.lostFrames,
      incompleteFrames: this.incompleteFrames,
      throughputMbps: (this.bytes * 8) / seconds / 1000000
    };
  }
}

module.exports = { VisualizerStats };
