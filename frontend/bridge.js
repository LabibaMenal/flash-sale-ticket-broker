// bridge.js — translates browser HTTP requests into TCP commands for the C++ engine
const http = require('http');
const net  = require('net');

// Update these lines near the top of your bridge.js file:
const CPP_HOST = 'hayabusa.proxy.rlwy.net';
const CPP_PORT = 51379; // No quotes here, keep it as a number!

const BRIDGE_PORT = 3000;

function setCors(res) {
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
}

function sendToEngine(command) {
    return new Promise((resolve, reject) => {
        const client = new net.Socket();
        let response = '';

        client.connect(CPP_PORT, CPP_HOST, () => {
            client.write(command + '\n');
        });

        client.on('data', (data) => {
            response += data.toString();
            client.destroy(); // one command per connection, matches C++ server design
        });

        client.on('close', () => resolve(response.trim()));
        client.on('error', (err) => reject(err));

        // Safety timeout in case the engine never responds
        setTimeout(() => {
            client.destroy();
            resolve(response.trim() || 'TIMEOUT: no response from engine');
        }, 3000);
    });
}

const server = http.createServer((req, res) => {
    setCors(res);

    if (req.method === 'OPTIONS') {
        res.writeHead(204);
        res.end();
        return;
    }

    if (req.method === 'POST' && req.url === '/book') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', async () => {
            try {
                const { userId, userName, tier } = JSON.parse(body);

                if (!userId || !userName || !tier) {
                    res.writeHead(400, { 'Content-Type': 'application/json; charset=utf-8' });
                    res.end(JSON.stringify({ error: 'Missing userId, userName, or tier' }));
                    return;
                }

                // Matches the exact protocol your C++ Server::parseCommand expects
                const command = `BOOK_TICKET ${userId} ${userName} ${tier.toUpperCase()}`;
                const engineResponse = await sendToEngine(command);

                res.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8' });
                res.end(JSON.stringify({ raw: engineResponse }));
            } catch (err) {
                res.writeHead(500, { 'Content-Type': 'application/json; charset=utf-8' });
                res.end(JSON.stringify({ error: err.message }));
            }
        });
        return;
    }

    res.writeHead(404, { 'Content-Type': 'application/json; charset=utf-8' });
    res.end(JSON.stringify({ error: 'Not found. POST to /book' }));
});

server.listen(BRIDGE_PORT, () => {
    console.log(`[Bridge] Listening on http://localhost:${BRIDGE_PORT}`);
    console.log(`[Bridge] Forwarding to C++ engine at ${CPP_HOST}:${CPP_PORT}`);
    console.log(`[Bridge] Make sure ticket_broker.exe --server is running first!`);
});
