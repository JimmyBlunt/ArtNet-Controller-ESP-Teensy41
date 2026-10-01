// Processing 4 sketch shell. Core packet parsing is mirrored in processing/src tests.
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.util.HashMap;

ArrayList<PVector> points = new ArrayList<PVector>();
color[] pixels = new color[0];
float zoom = 1.0;
boolean glow = false;
int performanceMode = 1;
boolean gridView = true;
String inputMode = "Controller Preview";
String connectionStatus = "UDP idle";
String mappingId = "demo";
int previewPort = 6455;
int artNetPort = 6454;
int artNetStartUniverse = 0;
int artNetUniverseCount = 2;
int expectedPixels = 256;
long receivedFrames = 0;
long renderedFrames = 0;
long incompleteFrames = 0;
long lostFrames = 0;
long receivedBytes = 0;
int lastFrameId = -1;
int lastLoggedFrameId = -1;
long startedMs;
UdpReceiver previewReceiver;
UdpReceiver artNetReceiver;
PreviewAssembler previewAssembler = new PreviewAssembler();
ArtNetAssembler artNetAssembler = new ArtNetAssembler(artNetStartUniverse, artNetUniverseCount, expectedPixels);
Object frameLock = new Object();
boolean customMappingLoaded = false;
String mappingPath = "";

void setup() {
  size(1200, 800, P3D);
  frameRate(60);
  if (!loadOutput0Matrix16()) {
  if (!loadMappingPoints()) {
    loadDemoPoints(1500);
  }
  }
  startedMs = millis();
  startReceivers();
  println("ArtNetMixedLedVisualizer ready previewPort=" + previewPort + " artNetPort=" + artNetPort);
}

void draw() {
  renderedFrames++;
  background(8);
  if (gridView) {
    drawPixelGrid();
  } else {
    lights();
    drawBounds();
    translate(width / 2, height / 2, -200);
    rotateY(frameCount * 0.003);
    scale(zoom);
    strokeWeight(glow ? 6 : 3);
    color[] frame;
    synchronized (frameLock) {
      frame = pixels.clone();
    }
    for (int i = 0; i < points.size(); i++) {
      PVector p = points.get(i);
      color c = i < frame.length ? frame[i] : color(24);
      if ((c & 0x00ffffff) == 0) c = color(18);
      stroke(c);
      point((p.x - 0.5) * 700, (p.y - 0.5) * 420, (p.z - 0.5) * 420);
    }
    hint(DISABLE_DEPTH_TEST);
    camera();
  }
  fill(240);
  text("Input: " + inputMode + "  Quality: " + performanceLabel(), 16, 24);
  text("Status: " + connectionStatus + "  Mapping: " + mappingId, 16, 44);
  text("Rendered FPS: " + nf(frameRate, 1, 1) + " Received FPS: " + nf(receiveFps(), 1, 1) + " Mapped: " + points.size() + " Frame: " + pixels.length + " View: " + (gridView ? "Grid" : "3D") + " Zoom: " + nf(zoom, 1, 2), 16, 64);
  text("Lost: " + lostFrames + " Incomplete: " + incompleteFrames + " Throughput Mbps: " + nf(throughputMbps(), 1, 2), 16, 84);
  text("Keys: p preview:" + previewPort + ", a art-net:" + artNetPort + ", v grid/3D, q/b/f quality, g glow, 1/2/3 pixel count", 16, 104);
  hint(ENABLE_DEPTH_TEST);
}

void mouseWheel(processing.event.MouseEvent event) {
  zoom = constrain(zoom - event.getCount() * 0.05, 0.1, 8.0);
}

void keyPressed() {
  if (key == 'g') glow = !glow;
  if (key == 'v') gridView = !gridView;
  if (key == 'p') inputMode = "Controller Preview";
  if (key == 'a') inputMode = "Direct Art-Net";
  if (key == 'b') inputMode = "Playback";
  if (key == 'q') performanceMode = 2;
  if (key == 'f') performanceMode = 0;
  if (key == '1') loadDemoPoints(1500);
  if (key == '2') loadDemoPoints(5000);
  if (key == '3') loadDemoPoints(10000);
}

void drawPixelGrid() {
  color[] frame;
  ArrayList<PVector> mappedPoints;
  synchronized (frameLock) {
    frame = pixels.clone();
    mappedPoints = new ArrayList<PVector>(points);
  }
  int count = max(1, frame.length);
  boolean useMappedGrid = mappedPoints.size() == frame.length && mappedPoints.size() > 0;
  int cols = useMappedGrid ? 16 : ceil(sqrt(count * 1.65));
  int rows = useMappedGrid ? 16 : ceil(count / (float) cols);
  float marginX = 16;
  float top = 130;
  float availableW = width - marginX * 2;
  float availableH = height - top - 20;
  float cell = min(availableW / cols, availableH / rows) * zoom;
  cell = max(1.0, cell);
  float gridW = cols * cell;
  float gridH = rows * cell;
  float startX = (width - gridW) / 2.0;
  float startY = top + max(0, (availableH - gridH) / 2.0);
  noStroke();
  for (int i = 0; i < count; i++) {
    color c = i < frame.length ? frame[i] : color(0);
    if ((c & 0x00ffffff) == 0) c = color(14);
    float x = startX + (i % cols) * cell;
    float y = startY + (i / cols) * cell;
    if (useMappedGrid) {
      PVector p = mappedPoints.get(i);
      x = startX + constrain(p.x, 0, 1) * (cols - 1) * cell;
      y = startY + constrain(p.y, 0, 1) * (rows - 1) * cell;
    }
    if (glow && (c & 0x00ffffff) != 0) {
      fill(red(c), green(c), blue(c), 110);
      rect(x - cell * 0.25, y - cell * 0.25, cell * 1.5, cell * 1.5);
    }
    fill(c);
    rect(x, y, max(1, cell - 1), max(1, cell - 1));
  }
  noFill();
  stroke(70);
  rect(startX, startY, gridW, gridH);
}

void startReceivers() {
  previewReceiver = new UdpReceiver(previewPort, new PacketHandler() {
    public void handle(byte[] data, int length) {
      handlePreviewPacket(data, length);
    }
  });
  artNetReceiver = new UdpReceiver(artNetPort, new PacketHandler() {
    public void handle(byte[] data, int length) {
      handleArtNetPacket(data, length);
    }
  });
  previewReceiver.start();
  artNetReceiver.start();
}

void stop() {
  if (previewReceiver != null) previewReceiver.close();
  if (artNetReceiver != null) artNetReceiver.close();
  super.stop();
}

void handlePreviewPacket(byte[] data, int length) {
  receivedBytes += length;
  PreviewChunk chunk = decodePreviewChunk(data, length);
  if (chunk == null) return;
  color[] frame = previewAssembler.accept(chunk);
  if (frame != null) {
    applyFrame(frame, "preview:" + chunk.mappingHash, chunk.frameId);
    connectionStatus = "Preview frame " + chunk.frameId;
  }
}

void handleArtNetPacket(byte[] data, int length) {
  receivedBytes += length;
  ArtDmxPacket packet = decodeArtDmxPacket(data, length);
  if (packet == null) return;
  color[] frame = artNetAssembler.accept(packet);
  if (frame != null) {
    applyFrame(frame, "direct-artnet", artNetAssembler.frameId);
    connectionStatus = "Art-Net frame " + artNetAssembler.frameId;
  }
}

void applyFrame(color[] frame, String newMappingId, int frameId) {
  synchronized (frameLock) {
    pixels = frame;
    if (!customMappingLoaded && points.size() != frame.length) loadDemoPoints(frame.length);
    mappingId = newMappingId;
  }
  if (lastFrameId >= 0 && frameId > lastFrameId + 1) lostFrames += frameId - lastFrameId - 1;
  lastFrameId = frameId;
  receivedFrames++;
  if (frameId != lastLoggedFrameId && (receivedFrames < 10 || frameId % 30 == 0)) {
    println("received frame source=" + newMappingId + " frameId=" + frameId + " pixels=" + frame.length);
    lastLoggedFrameId = frameId;
  }
}

String performanceLabel() {
  if (performanceMode == 0) return "Performance";
  if (performanceMode == 2) return "Quality";
  return "Balanced";
}

void drawBounds() {
  pushMatrix();
  translate(width / 2, height / 2, -200);
  scale(zoom);
  noFill();
  stroke(70);
  box(720, 440, 440);
  stroke(160, 80, 80);
  line(-360, 0, 0, 360, 0, 0);
  stroke(80, 160, 80);
  line(0, -220, 0, 0, 220, 0);
  stroke(80, 80, 180);
  line(0, 0, -220, 0, 0, 220);
  popMatrix();
}

void loadDemoPoints(int count) {
  synchronized (frameLock) {
    customMappingLoaded = false;
    mappingPath = "";
    points.clear();
    for (int i = 0; i < count; i++) {
      float a = TWO_PI * i / max(1, count);
      float radius = 0.15 + 0.35 * (i % 97) / 97.0;
      points.add(new PVector(0.5 + cos(a) * radius, 0.5 + sin(a) * radius, (i % 200) / 199.0));
    }
    if (pixels.length != count) {
      pixels = new color[count];
    }
  }
}

boolean loadMappingPoints() {
  String[] candidates = {
    "Mappings/schrankwand -onlyu.txt",
    "C:/Users/jimmy/Documents/~ pROJECTs ~/Schrank-LED/Mappings/schrankwand -onlyu.txt"
  };
  for (String candidate : candidates) {
    File file = new File(candidate);
    if (!file.exists()) continue;
    try {
      JSONArray json = loadJSONArray(candidate);
      if (json == null || json.size() == 0) continue;
      float minX = Float.MAX_VALUE;
      float minY = Float.MAX_VALUE;
      float minZ = Float.MAX_VALUE;
      float maxX = -Float.MAX_VALUE;
      float maxY = -Float.MAX_VALUE;
      float maxZ = -Float.MAX_VALUE;
      for (int i = 0; i < json.size(); i++) {
        JSONArray row = json.getJSONArray(i);
        if (row == null || row.size() < 3) continue;
        float x = row.getFloat(0);
        float y = row.getFloat(1);
        float z = row.getFloat(2);
        minX = min(minX, x);
        minY = min(minY, y);
        minZ = min(minZ, z);
        maxX = max(maxX, x);
        maxY = max(maxY, y);
        maxZ = max(maxZ, z);
      }
      float rangeX = max(0.0001, maxX - minX);
      float rangeY = max(0.0001, maxY - minY);
      float rangeZ = max(0.0001, maxZ - minZ);
      synchronized (frameLock) {
        points.clear();
        for (int i = 0; i < json.size(); i++) {
          JSONArray row = json.getJSONArray(i);
          if (row == null || row.size() < 3) continue;
          float x = (row.getFloat(0) - minX) / rangeX;
          float y = 1.0 - ((row.getFloat(1) - minY) / rangeY);
          float z = (row.getFloat(2) - minZ) / rangeZ;
          points.add(new PVector(x, y, z));
        }
        pixels = new color[points.size()];
        customMappingLoaded = true;
        gridView = false;
        mappingPath = candidate;
        mappingId = "schrankwand-onlyu";
      }
      println("loaded mapping " + candidate + " points=" + points.size()
        + " bounds=[" + minX + "," + minY + "," + minZ + "]-[" + maxX + "," + maxY + "," + maxZ + "]");
      return true;
    } catch (Exception e) {
      println("mapping load failed " + candidate + ": " + e.getMessage());
    }
  }
  println("mapping file not found, using demo points");
  return false;
}

boolean loadOutput0Matrix16() {
  synchronized (frameLock) {
    points.clear();
    for (int index = 0; index < 256; index++) {
      int row = index / 16;
      int wiredCol = index % 16;
      int col = row % 2 == 0 ? wiredCol : 15 - wiredCol;
      points.add(new PVector(col / 15.0, row / 15.0, 0.5));
    }
    pixels = new color[256];
    customMappingLoaded = true;
    gridView = true;
    mappingPath = "generated:output0-16x16-serpentine";
    mappingId = "output0-16x16-serpentine";
  }
  println("loaded generated mapping output0-16x16-serpentine points=256");
  return true;
}

float receiveFps() {
  return receivedFrames * 1000.0 / max(1, millis() - startedMs);
}

float throughputMbps() {
  return receivedBytes * 8.0 / max(1, millis() - startedMs) / 1000.0;
}

interface PacketHandler {
  void handle(byte[] data, int length);
}

class UdpReceiver extends Thread {
  int port;
  PacketHandler handler;
  boolean running = true;
  DatagramSocket socket;

  UdpReceiver(int port, PacketHandler handler) {
    this.port = port;
    this.handler = handler;
  }

  public void run() {
    try {
      socket = new DatagramSocket(port);
      socket.setSoTimeout(500);
      byte[] buffer = new byte[1600];
      while (running) {
        DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
        try {
          socket.receive(packet);
          byte[] copy = new byte[packet.getLength()];
          arrayCopy(packet.getData(), packet.getOffset(), copy, 0, packet.getLength());
          handler.handle(copy, copy.length);
        } catch (SocketTimeoutException timeout) {
          // Allow clean shutdown checks.
        }
      }
    } catch (SocketException e) {
      connectionStatus = "UDP bind failed on " + port + ": " + e.getMessage();
    } catch (Exception e) {
      connectionStatus = "UDP error on " + port + ": " + e.getMessage();
    } finally {
      if (socket != null) socket.close();
    }
  }

  void close() {
    running = false;
    if (socket != null) socket.close();
  }
}

class PreviewChunk {
  int frameId;
  int pixelCount;
  int chunkIndex;
  int chunkCount;
  int mappingHash;
  color[] chunkPixels;

}

PreviewChunk decodePreviewChunk(byte[] data, int length) {
  if (length < 25 || data[0] != 'S' || data[1] != 'L' || data[2] != 'P' || data[3] != 'V') return null;
  PreviewChunk chunk = new PreviewChunk();
  chunk.frameId = read32(data, 5);
  chunk.pixelCount = read32(data, 13);
  int packedChunk = read32(data, 17);
  chunk.chunkIndex = (packedChunk >>> 16) & 0xffff;
  chunk.chunkCount = packedChunk & 0xffff;
  chunk.mappingHash = read32(data, 21);
  int count = (length - 25) / 3;
  chunk.chunkPixels = new color[count];
  for (int i = 0; i < count; i++) {
    int offset = 25 + i * 3;
    chunk.chunkPixels[i] = color(data[offset] & 0xff, data[offset + 1] & 0xff, data[offset + 2] & 0xff);
  }
  return chunk;
}

class PreviewAssembler {
  HashMap<String, PreviewFrameState> frames = new HashMap<String, PreviewFrameState>();

  color[] accept(PreviewChunk chunk) {
    String key = chunk.frameId + ":" + chunk.mappingHash;
    PreviewFrameState state = frames.get(key);
    if (state == null) {
      state = new PreviewFrameState(chunk.pixelCount, chunk.chunkCount);
      frames.put(key, state);
    }
    if (chunk.chunkIndex >= 0 && chunk.chunkIndex < state.chunks.length && state.chunks[chunk.chunkIndex] == null) {
      state.chunks[chunk.chunkIndex] = chunk.chunkPixels;
      state.received++;
    }
    if (state.received != state.chunks.length) return null;
    color[] frame = new color[state.pixelCount];
    int target = 0;
    for (int i = 0; i < state.chunks.length; i++) {
      color[] part = state.chunks[i];
      for (int j = 0; j < part.length && target < frame.length; j++) {
        frame[target++] = part[j];
      }
    }
    frames.remove(key);
    return frame;
  }
}

class PreviewFrameState {
  int pixelCount;
  color[][] chunks;
  int received = 0;

  PreviewFrameState(int pixelCount, int chunkCount) {
    this.pixelCount = pixelCount;
    this.chunks = new color[max(1, chunkCount)][];
  }
}

class ArtDmxPacket {
  int universe;
  byte[] payload;
}

ArtDmxPacket decodeArtDmxPacket(byte[] data, int length) {
  if (length < 18) return null;
  String id = new String(data, 0, 8);
  if (!id.equals("Art-Net\u0000")) return null;
  int opcode = (data[8] & 0xff) | ((data[9] & 0xff) << 8);
  if (opcode != 0x5000) return null;
  int payloadLength = ((data[16] & 0xff) << 8) | (data[17] & 0xff);
  if (payloadLength <= 0 || payloadLength > 512 || length < 18 + payloadLength) return null;
  ArtDmxPacket packet = new ArtDmxPacket();
  packet.universe = (data[14] & 0xff) | ((data[15] & 0xff) << 8);
  packet.payload = new byte[payloadLength];
  arrayCopy(data, 18, packet.payload, 0, payloadLength);
  return packet;
}

class ArtNetAssembler {
  int startUniverse;
  int universeCount;
  int pixelCount;
  int frameId = 0;
  byte[][] universes;
  boolean[] received;
  int receivedCount = 0;

  ArtNetAssembler(int startUniverse, int universeCount, int pixelCount) {
    this.startUniverse = startUniverse;
    this.universeCount = universeCount;
    this.pixelCount = pixelCount;
    this.universes = new byte[universeCount][];
    this.received = new boolean[universeCount];
  }

  color[] accept(ArtDmxPacket packet) {
    int index = packet.universe - startUniverse;
    if (index < 0 || index >= universeCount) return null;
    if (!received[index]) {
      received[index] = true;
      receivedCount++;
    }
    universes[index] = packet.payload;
    if (receivedCount != universeCount) return null;
    color[] frame = new color[pixelCount];
    int pixel = 0;
    for (int u = 0; u < universeCount; u++) {
      byte[] payload = universes[u];
      for (int offset = 0; offset + 2 < payload.length && pixel < pixelCount; offset += 3) {
        frame[pixel++] = color(payload[offset] & 0xff, payload[offset + 1] & 0xff, payload[offset + 2] & 0xff);
      }
      received[u] = false;
      universes[u] = null;
    }
    receivedCount = 0;
    frameId++;
    return frame;
  }
}

int read32(byte[] data, int offset) {
  return (data[offset] & 0xff) |
    ((data[offset + 1] & 0xff) << 8) |
    ((data[offset + 2] & 0xff) << 16) |
    ((data[offset + 3] & 0xff) << 24);
}
