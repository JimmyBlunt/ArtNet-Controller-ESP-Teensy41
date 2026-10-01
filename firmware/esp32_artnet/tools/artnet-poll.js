#!/usr/bin/env node
const dgram = require("dgram");

function arg(name, fallback) {
  const index = process.argv.indexOf(name);
  return index >= 0 ? process.argv[index + 1] : fallback;
}

const bindAddress = arg("--bind", "10.0.0.173");
const target = arg("--target", "10.0.0.244");
const timeoutMs = Number(arg("--timeout", "3000"));

function artPollPacket() {
  const packet = Buffer.alloc(14);
  packet.write("Art-Net\0", 0, "ascii");
  packet.writeUInt16LE(0x2000, 8);
  packet.writeUInt16BE(14, 10);
  packet.writeUInt16LE(0x0000, 12);
  return packet;
}

function parseArtPollReply(message, remote) {
  const id = message.subarray(0, 8).toString("ascii");
  if (id !== "Art-Net\0" || message.length < 239) return null;
  const opcode = message.readUInt16LE(8);
  if (opcode !== 0x2100) return null;
  const ip = `${message[10]}.${message[11]}.${message[12]}.${message[13]}`;
  const port = message.readUInt16LE(14);
  const shortName = message.subarray(26, 44).toString("ascii").replace(/\0.*$/, "");
  const longName = message.subarray(44, 108).toString("ascii").replace(/\0.*$/, "");
  const netSwitch = message[18];
  const subSwitch = message[19];
  const numPorts = message.readUInt16BE(172);
  return { remote: `${remote.address}:${remote.port}`, ip, port, shortName, longName, netSwitch, subSwitch, numPorts };
}

async function main() {
  const socket = dgram.createSocket({ type: "udp4", reuseAddr: true });
  const replies = [];
  socket.on("message", (message, remote) => {
    const reply = parseArtPollReply(message, remote);
    if (reply) {
      replies.push(reply);
      console.log(JSON.stringify(reply));
    }
  });
  await new Promise((resolve, reject) => {
    socket.once("error", reject);
    socket.bind(6454, bindAddress, () => {
      socket.removeListener("error", reject);
      socket.setBroadcast(true);
      resolve();
    });
  });
  const packet = artPollPacket();
  socket.send(packet, 6454, target);
  socket.send(packet, 6454, "10.255.255.255");
  await new Promise(resolve => setTimeout(resolve, timeoutMs));
  socket.close();
  if (!replies.length) {
    console.error("no ArtPollReply received");
    process.exit(2);
  }
}

main().catch(error => {
  console.error(error);
  process.exit(1);
});
