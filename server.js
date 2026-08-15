const http = require('http');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

const PORT = process.env.PORT || 3000;
const PUBLIC_DIR = __dirname;

const MIME_TYPES = {
    '.html': 'text/html; charset=UTF-8',
    '.css': 'text/css',
    '.js': 'application/javascript',
    '.json': 'application/json',
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.svg': 'image/svg+xml',
    '.ico': 'image/x-icon'
};

const server = http.createServer((req, res) => {
    // Enable CORS
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    if (req.method === 'OPTIONS') {
        res.writeHead(204);
        res.end();
        return;
    }

    if (req.method === 'POST' && req.url === '/compile') {
        let body = '';
        req.on('data', chunk => { body += chunk.toString(); });
        req.on('end', () => {
            const tempFile = path.join(__dirname, `temp_${Date.now()}.drone`);
            const jsonFile = path.join(__dirname, `temp_${Date.now()}.json`);

            fs.writeFile(tempFile, body, (err) => {
                if (err) {
                    res.writeHead(500, { 'Content-Type': 'application/json' });
                    res.end(JSON.stringify({ error: 'Failed to write temp code file' }));
                    return;
                }

                const isWin = process.platform === 'win32';
                const exeName = isWin ? 'dronec.exe' : './dronec';
                const exePath = path.join(__dirname, exeName);
                const dronec = spawn(exePath, [tempFile, jsonFile]);

                dronec.on('close', (code) => {
                    fs.readFile(jsonFile, 'utf8', (readErr, data) => {
                        // Cleanup temp files
                        fs.unlink(tempFile, () => {});
                        fs.unlink(jsonFile, () => {});

                        if (readErr) {
                            res.writeHead(500, { 'Content-Type': 'application/json' });
                            res.end(JSON.stringify({
                                errors: ['Compilation executable failed to produce valid JSON output.'],
                                frames: [],
                                final_state: {}
                            }));
                            return;
                        }

                        try {
                            const parsed = JSON.parse(data);
                            res.writeHead(200, { 'Content-Type': 'application/json' });
                            res.end(JSON.stringify(parsed));
                        } catch (parseErr) {
                            res.writeHead(200, { 'Content-Type': 'application/json' });
                            res.end(JSON.stringify({
                                errors: ['JSON Parsing error from compiler output stream.'],
                                frames: [],
                                final_state: {}
                            }));
                        }
                    });
                });
            });
        });
        return;
    }

    // Static file handler
    let filePath = path.join(PUBLIC_DIR, req.url === '/' ? 'index.html' : req.url);
    const ext = path.extname(filePath).toLowerCase();
    const contentType = MIME_TYPES[ext] || 'application/octet-stream';

    fs.readFile(filePath, (err, content) => {
        if (err) {
            if (err.code === 'ENOENT') {
                res.writeHead(404, { 'Content-Type': 'text/html' });
                res.end('<h1>404 Not Found</h1>', 'utf-8');
            } else {
                res.writeHead(500);
                res.end(`Server Error: ${err.code}`);
            }
        } else {
            res.writeHead(200, { 'Content-Type': contentType });
            res.end(content, 'utf-8');
        }
    });
});

server.listen(PORT, () => {
    console.log(`AeroScript Server running at http://localhost:${PORT}`);
});
