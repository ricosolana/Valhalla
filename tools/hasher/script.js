function stable_hash(str) {
    var num = 5381 | 0;
    var num2 = 5381 | 0;
    var num3 = 0;

    while (num3 < str.length) {
        num = (((num << 5) + num) ^ str.charCodeAt(num3)) | 0;

        if (num3 === str.length - 1) {
            break;
        }

        num2 = (((num2 << 5) + num2) ^ str.charCodeAt(num3 + 1)) | 0;
        num3 += 2;
    }

    // Multiply using 32-bit signed math
    var result = (num + Math.imul(num2, 1566083941)) | 0;

    return result; // Proper signed 32-bit output
}

function convert() {
    //const input = document.getElementById("inputString").value;
    //const result = stable_hash_code5(input);
    //document.getElementById("output").innerText = result;

    const input = document.getElementById("inputString").value;
    const result = stable_hash(input);

    // Force to signed 32-bit
    const signed32 = result | 0;

    // Convert to unsigned for byte extraction
    const unsigned32 = signed32 >>> 0;

    // Extract 4 bytes (big-endian order)
    const byte1 = (unsigned32 >>> 24) & 0xFF;
    const byte2 = (unsigned32 >>> 16) & 0xFF;
    const byte3 = (unsigned32 >>> 8) & 0xFF;
    const byte4 = unsigned32 & 0xFF;

    const hexNumber = "0x" + unsigned32.toString(16).padStart(8, "0").toUpperCase();

    const hexBytes =
        byte1.toString(16).padStart(2, "0").toUpperCase() + " " +
        byte2.toString(16).padStart(2, "0").toUpperCase() + " " +
        byte3.toString(16).padStart(2, "0").toUpperCase() + " " +
        byte4.toString(16).padStart(2, "0").toUpperCase();

    document.getElementById("output").innerHTML =
        `
        Decimal (signed 32-bit): ${signed32}<br>
        Hex (32-bit): ${hexNumber}<br>
        Bytes (big-endian): ${hexBytes}
        `;
}

// ---------- Test Cases ----------
function runTests() {
    const tests = [
        { input: "ClientHandshake", expected: 1021693670 },
        { input: "PeerInfo", expected: -725574882 },
        { input: "PlayerList", expected: -265949079 },
        { input: "ServerSyncedPlayerData", expected: 542500494 },
    ];

    const testResults = document.getElementById("testResults");
    const testsContainer = document.getElementById("tests");
    const warningBox = document.getElementById("warningBox");

    let hasFailure = false;

    tests.forEach(test => {
        const actual = stable_hash(test.input);
        const li = document.createElement("li");

        if (actual === test.expected) {
            li.innerText = `PASS: "${test.input}" → ${actual}`;
            li.className = "pass";
        } else {
            li.innerText = `FAIL: "${test.input}" → Expected ${test.expected}, got ${actual}`;
            li.className = "fail";
            hasFailure = true;
        }

        testResults.appendChild(li);
    });

    // Only show tests if something failed
    if (hasFailure) {
        testsContainer.style.display = "block";
        warningBox.style.display = "block";
    }
}

window.onload = runTests;