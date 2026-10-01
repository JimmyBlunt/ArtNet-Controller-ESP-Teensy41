const { run } = require("./test_web_ui_playwright");
run(true).catch(error => { console.error(error); process.exitCode = 1; });
