
import tailwindcss from '@tailwindcss/vite';
import react from '@vitejs/plugin-react';
import path from 'path';
import { defineConfig, type Plugin } from 'vite';

function wokwiTelemetryBridge(): Plugin {
  let latest: {
    telemetry: Record<string, unknown>;
    receivedAt: number;
  } | null = null;

  return {
    name: 'wokwi-telemetry-bridge',

    configureServer(server) {
      server.middlewares.use('/api/wokwi/telemetry', (req, res) => {
        if (req.method !== 'POST') {
          res.statusCode = 405;
          res.setHeader('Allow', 'POST');
          res.end('POST only');
          return;
        }

        let body = '';
        let tooLarge = false;

        req.on('data', (chunk) => {
          body += chunk.toString();

          if (body.length > 8192 && !tooLarge) {
            tooLarge = true;
            res.statusCode = 413;
            res.end('Payload too large');
            req.destroy();
          }
        });

        req.on('end', () => {
          if (tooLarge) return;

          try {
            const data = JSON.parse(body);

            const valid =
              typeof data.temperature === 'number' &&
              typeof data.humidity === 'number' &&
              typeof data.motion === 'boolean' &&
              typeof data.distance === 'number' &&
              (data.alarm === undefined ||
                typeof data.alarm === 'boolean') &&
              (data.armed === undefined ||
                typeof data.armed === 'boolean');

            if (!valid) {
              res.statusCode = 400;
              res.end('Invalid telemetry');
              return;
            }

            latest = {
              telemetry: data,
              receivedAt: Date.now(),
            };

            console.log('[Wokwi] Telemetry received:', data);

            res.statusCode = 200;
            res.setHeader('Content-Type', 'application/json');
            res.setHeader('Cache-Control', 'no-store');
            res.end(
              JSON.stringify({
                ok: true,
                receivedAt: latest.receivedAt,
              })
            );
          } catch {
            res.statusCode = 400;
            res.end('Invalid JSON');
          }
        });

        req.on('error', () => {
          if (!res.writableEnded) {
            res.statusCode = 400;
            res.end('Request error');
          }
        });
      });

      server.middlewares.use('/api/wokwi/latest', (req, res) => {
        if (req.method !== 'GET') {
          res.statusCode = 405;
          res.setHeader('Allow', 'GET');
          res.end('GET only');
          return;
        }

        res.statusCode = 200;
        res.setHeader('Content-Type', 'application/json');
        res.setHeader('Cache-Control', 'no-store');
        res.end(JSON.stringify(latest ?? {}));
      });
    },
  };
}

export default defineConfig(() => ({
  plugins: [
    react(),
    tailwindcss(),
    wokwiTelemetryBridge(),
  ],

  resolve: {
    alias: {
      '@': path.resolve(__dirname, '.'),
    },
  },

  server: {
    host: '0.0.0.0',

    // Allow requests through the current Cloudflare quick tunnel.
    allowedHosts: [
      'pins-erik-palmer-valium.trycloudflare.com',
    ],

    // Preserve the existing HMR configuration.
    hmr: process.env.DISABLE_HMR !== 'true',
    watch: process.env.DISABLE_HMR === 'true' ? null : {},
  },
}));
