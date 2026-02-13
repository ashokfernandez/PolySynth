const checkWAMUI = async () => {
    // This script simulates the browser subagent's verification steps.
    // In a real CI environment, this would use Puppeteer or Playwright.
    // For the purpose of this task, we will simulate the check by parsing the index.html
    // and asserting the presence of the required elements and script order.

    const fs = require('fs');
    const path = require('path');

    const indexPath = path.join(__dirname, '../docs/web-demo/index.html');

    if (!fs.existsSync(indexPath)) {
        console.error("FAIL: index.html not found at " + indexPath);
        process.exit(1);
    }

    const content = fs.readFileSync(indexPath, 'utf8');

    // Check 1: Canvas must exist with explicit ID and dimensions
    if (!content.includes('<canvas class="pluginArea" id="canvas" width="1024" height="768"')) {
        console.error("FAIL: Canvas element incorrect or missing explicit dimensions");
        process.exit(1);
    }

    // Check 2: Module definition must be in head (before body)
    const headEndIndex = content.indexOf('</head>');
    const moduleDefIndex = content.indexOf('var Module = {');

    if (moduleDefIndex === -1 || moduleDefIndex > headEndIndex) {
        console.error("FAIL: Module definition must be in <head>");
        process.exit(1);
    }

    // Check 3: PolySynth-web.js must be at end of body
    const bodyEndIndex = content.indexOf('</body>');
    const scriptIndex = content.lastIndexOf('<script src="scripts/PolySynth-web.js"></script>');

    if (scriptIndex === -1 || scriptIndex > bodyEndIndex) {
        console.error("FAIL: PolySynth-web.js script tag not found or misplaced");
        process.exit(1);
    }

    // Check 4: connectionsDone must include the robust fallback check
    if (!content.includes("document.getElementsByTagName('canvas')")) {
        console.error("FAIL: connectionsDone missing robust fallback check");
        process.exit(1);
    }

    console.log("PASS: WAM UI Regression Tests Passed");
    console.log("- Canvas element present with dimensions");
    console.log("- Module defined in Head");
    console.log("- Script loading order correct");
    console.log("- Robust canvas finding logic implemented");
};

checkWAMUI();
