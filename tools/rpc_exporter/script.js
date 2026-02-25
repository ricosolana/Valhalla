// Force reset UI state on page reload
window.addEventListener("load", () => {
    outputBox.value = "";
    allFiles = [];
    dropText.textContent = "Drag files or folders here";
    processBtn.disabled = true;

    outputSection.style.display = "none";
    outputTitle.style.display = "none";
});

const dropZone = document.getElementById("dropZone");
const dropText = document.getElementById("dropText");
const fileInput = document.getElementById("fileInput");
const browseBtn = document.getElementById("browseBtn");
const processBtn = document.getElementById("processBtn");
const outputBox = document.getElementById("outputBox"); // still needed?
const outputSection = document.getElementById("outputSection");
const outputTitle = document.getElementById("outputTitle");
const copyBtn = document.getElementById("copyBtn");

let allFiles = [];

/* ---------------- UI Helpers ---------------- */

function updateSelectionDisplay() {
    if (allFiles.length === 0) {
        dropText.textContent = "Drag files or folders here";
        processBtn.disabled = true;
        return;
    }

    if (allFiles.length === 1) {
        dropText.textContent = `Selected: ${allFiles[0].webkitRelativePath || allFiles[0].name}`;
    } else {
        dropText.textContent = `Selected ${allFiles.length} files`;
    }

    processBtn.disabled = false;
}

/* ---------------- Drag & Drop ---------------- */

dropZone.addEventListener("dragover", (e) => {
    e.preventDefault();
    dropZone.classList.add("dragover");
});

dropZone.addEventListener("dragleave", () => {
    dropZone.classList.remove("dragover");
});

dropZone.addEventListener("drop", async (e) => {
    e.preventDefault();
    dropZone.classList.remove("dragover");

    const items = e.dataTransfer.items;
    allFiles = [];

    for (let item of items) {
        const entry = item.webkitGetAsEntry();
        if (entry) {
            await traverseFileTree(entry);
        }
    }

    updateSelectionDisplay();
});

async function traverseFileTree(item) {
    if (item.isFile) {
        await new Promise(resolve => {
            item.file(file => {
                allFiles.push(file);
                resolve();
            });
        });
    } else if (item.isDirectory) {
        const dirReader = item.createReader();
        await new Promise(resolve => {
            dirReader.readEntries(async (entries) => {
                for (let entry of entries) {
                    await traverseFileTree(entry);
                }
                resolve();
            });
        });
    }
}

/* ---------------- Browse ---------------- */

browseBtn.addEventListener("click", () => {
    fileInput.click();
});

fileInput.addEventListener("change", () => {
    allFiles = Array.from(fileInput.files);
    updateSelectionDisplay();
});

/* ---------------- Copy Button ---------------- */

copyBtn.addEventListener("click", async () => {
    if (!outputBox.value) return;

    await navigator.clipboard.writeText(outputBox.value);
    copyBtn.textContent = "Copied!";
    setTimeout(() => copyBtn.textContent = "Copy", 1500);
});

/* ---------------- Stable Hash ---------------- */

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

/* ---------------- Processing ---------------- */

processBtn.addEventListener("click", async () => {

    const sortedFiles = [...allFiles].sort((a, b) =>
        a.name.localeCompare(b.name, undefined, { sensitivity: "base" })
    );

    let clazzes = {};
    let clazzName = "";
    let gameVersion = "";

    const classRegex = / class (\w+)/;
    const registerPattern = /\.Register.*?\(([a-zA-Z0-9_"]+?),[a-zA-Z0-9 .]+(<[a-zA-Z0-9, <>]+>)?.*?\(([a-zA-Z0-9_.]+)/;

    for (let file of sortedFiles) {
        const text = await file.text();
        const lines = text.split(/\r?\n/);

        for (let line of lines) {

            const classMatch = line.match(classRegex);
            if (classMatch) {
                clazzName = classMatch[1];
                continue;
            }

            const regMatch = line.match(registerPattern);
            if (regMatch) {
                let hashName = regMatch[1].replace(/"/g, "");
                let params = regMatch[2] ? regMatch[2].slice(1, -1) : "";
                let methodName = regMatch[3].replace("this.", "");

                let regType = "";
                if (params.includes("ZRpc")) {
                    regType = "ZRpc";
                } else if (line.includes("oute")) {
                    regType = "ZRoutedRpc";
                } else {
                    regType = "ZNetView";
                }

                if (!clazzes[clazzName]) {
                    clazzes[clazzName] = {};
                }

                clazzes[clazzName][hashName] = {
                    methodName,
                    params,
                    regType,
                    hash: stable_hash(hashName)
                };
            }

            if (!gameVersion && clazzName === "Version" && line.includes("CurrentVersion")) {
                const match = line.match(/GameVersion\((\d+),\s*(\d+),\s*(\d+)\)/);
                if (match) {
                    gameVersion = `v${match[1]}.${match[2]}.${match[3]}`;
                }
            }
        }
    }

    let tableStr = "";

    for (let clazz in clazzes) {
        const rpcEntries = clazzes[clazz];
        const keys = Object.keys(rpcEntries).sort();

        if (keys.length === 0) continue;

        tableStr += `**${clazz}**\n`;
        tableStr += `| Name | Method | Params | Registers to | Hash |\n`;
        tableStr += `|-|-|-|-|-|\n`;

        for (let key of keys) {
            const entry = rpcEntries[key];
            tableStr += `| ${key} | ${entry.methodName} | ${entry.params} | ${entry.regType} | ${entry.hash} |\n`;
        }

        tableStr += `\n`;
    }

    tableStr += `Automatically generated by avl-rpc-wiki-gen Valheim ${gameVersion}`;
    outputBox.value = tableStr;

    outputSection.style.display = "block";
    outputTitle.style.display = "block";
});