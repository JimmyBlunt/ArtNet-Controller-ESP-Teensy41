const assert = require("assert");
const fs = require("fs");
const path = require("path");
const http = require("http");
const { chromium } = require("playwright");
const { bundle } = require("../tools/build-web-ui");

function fixture() {
  return {pixelCount:1679,startUniverse:0,universeCount:10,targetFps:30,outputs:[
    {id:0,type:"WS2812B",enabled:true,pixelCount:512,startPixel:0,dataPin:23,clockPin:-1,colorOrder:"GRB",reverse:false,spiHz:0,startUniverse:0,targetFps:30},
    {id:1,type:"APA102",enabled:true,pixelCount:1167,startPixel:512,dataPin:18,clockPin:19,colorOrder:"BGR",reverse:false,spiHz:4000000,startUniverse:3,targetFps:30}
  ]};
}
async function run(embedded = false) {
  let config=fixture(), saved=null, unavailable=false, rejectApply=false;
  const actions=[], posts=[];
  let test={active:false,loop:false,blackout:false,phase:"stopped",outputId:-1};
  const errors=[];
  const browser=await chromium.launch({headless:true});
  let server;
  try {
    const page=await browser.newPage({viewport:{width:1440,height:1000}});
    page.on("pageerror",e=>errors.push(e.message));
    await page.route("**/api/**",async route=>{
      if(unavailable) {await route.abort();return;}
      const req=route.request(), url=new URL(req.url()), method=req.method();
      let body={},status=200;
      if(method==="POST") body=JSON.parse(req.postData()||"{}");
      let result;
      if(url.pathname==="/api/status") result={pixelCount:config.pixelCount,universeCount:config.universeCount,startUniverse:config.startUniverse,fps:0,packets:0,packetsPerSecond:0,framesComplete:0,framesIncomplete:0,heapFree:160000,outputTimeUs:0,outputFrames:1234};
      else if(url.pathname==="/api/config") {
        if(method==="POST") {
          if(rejectApply) {status=400; result={error:"Unsupported output configuration"};}
          else {posts.push(body);config=structuredClone(body);result=config;}
        } else result=config;
      } else if(url.pathname==="/api/storage") result={saved:!!saved,matches:!!saved&&JSON.stringify(saved)===JSON.stringify(config)};
      else if(url.pathname==="/api/config/save") {saved=structuredClone(config);result={saved:true,matches:true};}
      else if(url.pathname==="/api/test-pattern") {
        if(method==="POST"){
          actions.push(body);
          test={active:["start","loop"].includes(body.action),loop:body.action==="loop",blackout:body.action==="blackout",phase:body.action==="loop"?"all-green-chase":body.action==="start"?"all-red-chase":body.action,outputId:body.outputId};
        }
        result=test;
      } else if(url.pathname==="/api/preview/target") result=body;
      else {status=404;result={error:"unexpected endpoint"};}
      await route.fulfill({status,contentType:"application/json",body:JSON.stringify(result)});
    });
    if(embedded) {
      server=http.createServer((req,res)=>{res.writeHead(200,{"Content-Type":"text/html; charset=utf-8"});res.end(bundle());});
      await new Promise(resolve=>server.listen(0,"127.0.0.1",resolve));
      await page.goto("http://127.0.0.1:"+server.address().port);
    } else {
      await page.goto(require("url").pathToFileURL(path.resolve("web/index.html")).href);
      assert.strictEqual(await page.locator(".output-card").count(),0,"offline has no fictional outputs");
      assert.strictEqual(await page.locator("#blackout").isDisabled(),true);
      assert((await page.locator("#railState").textContent()).includes("Nicht verbunden"));
      await page.fill("#controllerHost","controller.test");
      await page.click("#connectController");
    }
    await page.waitForFunction(()=>document.querySelector("#connectionStatus").textContent==="Verbunden");
    assert.strictEqual(await page.locator("#connectController").textContent(),"Neu verbinden");
    assert.strictEqual(await page.locator(".output-card").count(),2);
    assert.strictEqual(await page.locator("#testDeckTitle").textContent(),"Alle 2 Ausgänge testen");
    assert((await page.locator("#outputCards").textContent()).includes("512"));
    assert((await page.locator("#outputCards").textContent()).includes("1.167"));
    await page.click('[data-test-output="1"]');
    await page.waitForFunction(()=>document.querySelector("#testPhase").textContent.includes("red"));
    assert.deepStrictEqual(actions.at(-1),{action:"start",outputId:1});
    assert((await page.locator("#testVisualState").textContent()).includes("Rot"));
    assert((await page.locator("#testVisualTarget").textContent()).includes("Output 1"));
    assert((await page.locator("#testVisualDetail").textContent()).includes("1.234"));
    assert((await page.locator("#testVisual").getAttribute("class")).includes("running"));
    await page.click('[data-action="start"]');
    await page.waitForFunction(()=>document.querySelector("#testVisualTarget").textContent.includes("Alle aktiven"));
    await page.click('[data-action="loop"]');
    await page.waitForFunction(()=>document.querySelector("#testVisualState").textContent.includes("Grün"));
    assert.strictEqual(await page.locator('[data-action="loop"]').textContent(),"Loop läuft");
    assert.strictEqual(await page.locator('[data-action="loop"]').getAttribute("aria-pressed"),"true");
    assert.deepStrictEqual(actions.slice(-2).map(x=>x.action),["start","loop"]);
    await page.click("#blackout");
    await page.waitForFunction(()=>document.querySelector("#testPhase").textContent.includes("Blackout"));
    await page.click('[data-action="stop"]');
    await page.waitForFunction(()=>document.querySelector("#testPhase").textContent.includes("Art-Net freigegeben"));
    assert.deepStrictEqual(actions.slice(-2).map(x=>x.action),["blackout","stop"]);

    await page.click('[data-page="outputs"]');
    const pixels=page.locator('.outedit').first().locator('[data-k="pixelCount"]');
    await pixels.fill("");
    await pixels.pressSequentially("712",{delay:70});
    await page.waitForTimeout(1800);
    assert.strictEqual(await pixels.inputValue(),"712","polling preserves multi-digit typing and unsaved edits");
    assert.strictEqual(await page.locator("#saveConfig").isDisabled(),true,"dirty form cannot be saved before apply");
    await page.click("#autoLayout");
    assert((await page.locator("#configMsg").textContent()).includes("jetzt Anwenden"));
    assert.strictEqual(await page.locator("#cfgPixels").inputValue(),"1879");
    assert.strictEqual(await page.locator("#cfgUniverseCount").inputValue(),"12");
    assert.strictEqual(await page.locator('.outedit').nth(1).locator('[data-k="startPixel"]').inputValue(),"712");
    assert.strictEqual(await page.locator('.outedit').nth(1).locator('[data-k="startUniverse"]').inputValue(),"5");
    await page.click("#applyConfig");
    await page.waitForFunction(()=>document.querySelector("#configMsg").textContent.includes("Angewendet"));
    assert.strictEqual(posts.at(-1).outputs[1].startPixel,712);
    assert.strictEqual(posts.at(-1).outputs[1].startUniverse,5);
    assert.strictEqual(posts.at(-1).pixelCount,1879);
    await page.click("#saveConfig");
    await page.waitForFunction(()=>document.querySelector("#storageState").textContent==="Dauerhaft gespeichert");
    assert.strictEqual(saved.pixelCount,1879);

    await pixels.fill("600");
    rejectApply=true;
    await page.click("#applyConfig");
    await page.waitForFunction(()=>document.querySelector("#notice").textContent.includes("Unsupported"));
    assert.strictEqual(await pixels.inputValue(),"600","failed apply retains edits");
    assert.strictEqual(await page.locator("#saveConfig").isDisabled(),true);
    rejectApply=false;
    await page.click("#reloadConfig");
    await page.waitForFunction(()=>document.querySelector("#configMsg").textContent.includes("Neu geladen"));
    assert.strictEqual(await pixels.inputValue(),"712");

    config={pixelCount:256,startUniverse:0,universeCount:2,targetFps:30,hardwareProfile:"flex8-ws2812-apa102",outputs:[
      {id:0,type:"WS2812B",enabled:true,pixelCount:256,startPixel:0,dataPin:32,clockPin:-1,colorOrder:"GRB",reverse:false,spiHz:0,startUniverse:0,targetFps:30}
    ]};
    saved=null;
    await page.click("#connectController");
    await page.waitForFunction(()=>document.querySelectorAll(".outedit").length===1);
    assert.strictEqual(await page.locator("#testDeckTitle").textContent(),"Aktiven Ausgang testen");
    assert.strictEqual(await page.locator("#addOutput").isDisabled(),false,"flex profile enables add-output button");
    await page.click("#addOutput");
    assert.strictEqual(await page.locator(".outedit").count(),2,"user can add an output");
    assert.strictEqual(await page.locator('[data-action="loop"]').isDisabled(),true,"draft outputs cannot be tested before apply");
    assert((await page.locator("#notice").textContent()).includes("Zuerst Anwenden"));
    const second=page.locator(".outedit").nth(1);
    assert.strictEqual(await second.locator('[data-pin-field="apa"]').isHidden(),true,"unused APA pin pair is hidden for WS2812");
    await second.locator('[data-k="type"]').selectOption("APA102");
    assert.strictEqual(await second.locator('[data-pin-field="ws"]').isHidden(),true,"unused WS pin is hidden for APA102");
    assert.strictEqual(await second.locator('[data-pin-field="apa"]').isVisible(),true,"APA pin pair appears after type selection");
    await second.locator('[data-k="pinPair"]').selectOption("18/19");
    assert.strictEqual(await second.locator('[data-k="colorOrder"] option').count(),6);
    await second.locator('[data-k="colorOrder"]').selectOption("RGB");
    await page.locator('.outedit').first().locator('[data-k="colorOrder"]').selectOption("BRG");
    await second.locator('[data-k="startUniverse"]').fill("20");
    assert.strictEqual(await second.locator('[data-k="startUniverse"]').inputValue(),"20","individual start universe is editable");
    await page.click("#autoLayout");
    assert.strictEqual(await second.locator('[data-k="startUniverse"]').inputValue(),"2","auto layout assigns the next free universe range");
    await page.click("#applyConfig");
    await page.waitForFunction(()=>document.querySelector("#configMsg").textContent.includes("Angewendet"));
    assert.strictEqual(posts.at(-1).outputs[1].type,"APA102");
    assert.deepStrictEqual([posts.at(-1).outputs[1].dataPin,posts.at(-1).outputs[1].clockPin],[18,19]);
    assert.strictEqual(posts.at(-1).pixelCount,512);
    assert.deepStrictEqual(posts.at(-1).outputs.map(o=>o.colorOrder),["BRG","RGB"],"apply preserves independent color orders after auto layout");
    await page.click("#saveConfig");
    await page.waitForFunction(()=>document.querySelector("#configMsg").textContent.includes("gespeichert"));
    await page.click("#reloadConfig");
    await page.waitForFunction(()=>document.querySelector("#configMsg").textContent.includes("Neu geladen"));
    assert.deepStrictEqual(saved.outputs.map(o=>o.colorOrder),["BRG","RGB"]);
    assert.strictEqual(await second.locator('[data-k="colorOrder"]').inputValue(),"RGB");
    await page.click("#addOutput");
    assert.strictEqual(await page.locator(".outedit").count(),3);
    await page.locator('[data-remove-output="2"]').click();
    assert.strictEqual(await page.locator(".outedit").count(),2,"user can remove an output");

    config={pixelCount:256,startUniverse:0,universeCount:2,targetFps:30,hardwareProfile:"esp32-wroom-flex-8ws-2apa",outputs:[
      {id:0,type:"WS2812B",enabled:true,pixelCount:256,startPixel:0,dataPin:25,clockPin:-1,colorOrder:"GRB",reverse:false,spiHz:0,startUniverse:0,targetFps:30}
    ]};
    await page.click("#connectController");
    await page.waitForFunction(()=>document.querySelector("#hardwareNote").textContent.includes("G34"));
    const extensionOutput=page.locator(".outedit").first();
    assert.deepStrictEqual(await extensionOutput.locator('[data-k="dataPin"] option').evaluateAll(options=>options.map(option=>Number(option.value))),
      [25,26,27,14,19,18,5,17],"profile exposes both photographed four-GPIO groups");
    assert((await extensionOutput.locator('[data-k="dataPin"] option').allTextContents()).every(label=>label.includes("G")&&label.includes("GPIO")),
      "extension ports use board and GPIO labels");
    await extensionOutput.locator('[data-k="type"]').selectOption("APA102");
    assert.deepStrictEqual(await extensionOutput.locator('[data-k="pinPair"] option').evaluateAll(options=>options.map(option=>option.value)),
      ["18/5","26/27"],"APA102 offers both photographed-board pin pairs");
    assert(!(await page.locator("#hardwareNote").textContent()).includes("Ausgang verfügbar: G34"));
    assert((await page.locator("#hardwareNote").textContent()).includes("1.200 LEDs"));
    assert.strictEqual(await extensionOutput.locator('[data-k="pixelCount"]').getAttribute("max"),"1200");

    await page.click('[data-page="system"]');
    const downloadPromise=page.waitForEvent("download");
    await page.click("#exportConfig");
    const download=await downloadPromise;
    assert(download.suggestedFilename().startsWith("artnet-led-backup-"));
    const backupPath=await download.path();
    const backup=JSON.parse(fs.readFileSync(backupPath,"utf8"));
    assert.strictEqual(backup.schema,"artnet-led-controller-backup");
    assert.strictEqual(backup.version,1);
    assert.strictEqual(backup.config.outputs[0].dataPin,25);
    const restored={...backup,config:{...backup.config,pixelCount:128,universeCount:1,outputs:[
      {...backup.config.outputs[0],pixelCount:128,startPixel:0,startUniverse:7}
    ]}};
    page.once("dialog",dialog=>dialog.accept());
    await page.locator("#importConfigFile").setInputFiles({name:"controller-backup.json",mimeType:"application/json",buffer:Buffer.from(JSON.stringify(restored))});
    await page.waitForFunction(()=>document.querySelector("#backupMsg").textContent.includes("Wiederhergestellt"));
    assert.strictEqual(posts.at(-1).outputs[0].pixelCount,128);
    assert.strictEqual(posts.at(-1).outputs[0].startUniverse,7);
    assert.strictEqual(posts.at(-1).hardwareProfile,"esp32-wroom-flex-8ws-2apa");
    assert.strictEqual(saved.pixelCount,128,"restore persists the imported controller config");

    await page.click('[data-page="mapping"]');
    assert((await page.locator("#patchOverview").textContent()).includes("Startkanal"));
    await page.fill("#previewHost","10.0.0.173");
    await page.click("#savePreview");
    await page.waitForFunction(()=>document.querySelector("#previewMsg").textContent.includes("10.0.0.173:6455"));

    await page.click('[data-page="overview"]');
    const screenshotDir=path.resolve("output/playwright");
    fs.mkdirSync(screenshotDir,{recursive:true});
    await page.screenshot({path:path.join(screenshotDir,embedded?"controller-embedded-fixture.png":"controller-desktop-fixture.png"),fullPage:true});
    await page.setViewportSize({width:390,height:900});
    assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1),"mobile has no horizontal page overflow");
    await page.screenshot({path:path.join(screenshotDir,embedded?"controller-embedded-mobile-fixture.png":"controller-mobile-fixture.png"),fullPage:true});

    unavailable=true;
    await page.waitForFunction(()=>document.querySelector("#connectionStatus").textContent==="Offline",{},{timeout:10000});
    assert.strictEqual(await page.locator("#blackout").isDisabled(),true);
    assert((await page.locator("#railState").textContent()).includes("Nicht verbunden"));
    assert((await page.locator("#testPhase").textContent()).includes("unbekannt"));
    unavailable=false;
    await page.waitForFunction(()=>document.querySelector("#connectionStatus").textContent==="Verbunden",{},{timeout:10000});
    assert.deepStrictEqual(errors,[],"no uncaught browser errors");
    console.log((embedded?"Embedded":"Desktop")+" shared UI browser tests passed");
  } finally {await browser.close();if(server)await new Promise(resolve=>server.close(resolve));}
}
module.exports={run,fixture};
if(require.main===module)run().catch(error=>{console.error(error);process.exitCode=1;});
