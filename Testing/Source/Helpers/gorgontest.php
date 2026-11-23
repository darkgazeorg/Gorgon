<?php
/**
 * Gorgon HTTP Test Endpoint
 * Upload to: https://darkgaze.org/testing/gorgontest.php
 *
 * Supports the following query actions via GET parameter "action":
 *
 *   echo_headers   - Returns all received request headers as JSON
 *   echo_post      - Returns the raw POST body
 *   echo_base64    - Base64-decodes the POST body and returns raw binary bytes
 *   echo_all       - Returns JSON with both headers and POST body
 *   set_cookie     - Sets cookies specified in query params "name" and "value"
 *                    Optionally accepts "path", "domain", "secure", "httponly",
 *                    "samesite", "maxage" query params for cookie attributes
 *   check_cookie   - Returns the value of cookie specified by "name" query param
 *   echo_cookies   - Returns all received cookies as JSON
 *   echo_method    - Returns the HTTP method used
 *   set_response_header - Sets a response header from "name" and "value" query params
 *   multi          - Performs multiple actions separated by comma in "actions" param
 *   slow_download  - Streams a file ("file", default generated.bmp) over "seconds" (default 5)
 *
 * Default (no action): returns "OK"
 */

header('Content-Type: text/plain; charset=utf-8');

$action = isset($_GET['action']) ? $_GET['action'] : '';

function get_all_request_headers_assoc() {
    $headers = [];
    if (function_exists('getallheaders')) {
        foreach (getallheaders() as $name => $value) {
            $headers[$name] = $value;
        }
    } else {
        // Fallback for non-Apache
        foreach ($_SERVER as $key => $value) {
            if (strpos($key, 'HTTP_') === 0) {
                $name = str_replace('_', '-', substr($key, 5));
                $name = ucwords(strtolower($name), '-');
                $headers[$name] = $value;
            }
        }
    }
    return $headers;
}

function handle_action($action) {
    switch ($action) {
        case 'echo_headers':
            header('Content-Type: application/json; charset=utf-8');
            echo json_encode(get_all_request_headers_assoc(), JSON_PRETTY_PRINT);
            break;

        case 'echo_post':
            $body = file_get_contents('php://input');
            echo $body;
            break;

        case 'echo_base64':
            header('Content-Type: application/octet-stream');
            $body = file_get_contents('php://input');
            $decoded = base64_decode($body, true);
            if ($decoded === false) {
                http_response_code(400);
                echo 'Invalid base64';
            } else {
                echo $decoded;
            }
            break;

        case 'echo_all':
            header('Content-Type: application/json; charset=utf-8');
            $body = file_get_contents('php://input');
            echo json_encode([
                'method'  => $_SERVER['REQUEST_METHOD'],
                'headers' => get_all_request_headers_assoc(),
                'post'    => $body,
                'get'     => $_GET,
            ], JSON_PRETTY_PRINT);
            break;

        case 'set_cookie':
            $name  = isset($_GET['name'])  ? $_GET['name']  : 'testcookie';
            $value = isset($_GET['value']) ? $_GET['value'] : 'testvalue';
            $path  = isset($_GET['path'])  ? $_GET['path']  : '/';
            
            $options = [
                'path' => $path,
            ];
            
            if (isset($_GET['domain']))   $options['domain']   = $_GET['domain'];
            if (isset($_GET['maxage']))   $options['expires']  = time() + intval($_GET['maxage']);
            if (isset($_GET['secure']))   $options['secure']   = $_GET['secure'] === '1';
            if (isset($_GET['httponly'])) $options['httponly']  = $_GET['httponly'] === '1';
            if (isset($_GET['samesite'])) $options['samesite'] = $_GET['samesite'];
            
            setcookie($name, $value, $options);
            echo "cookie_set:" . $name . "=" . $value;
            break;

        case 'check_cookie':
            $name = isset($_GET['name']) ? $_GET['name'] : 'testcookie';
            if (isset($_COOKIE[$name])) {
                echo $_COOKIE[$name];
            } else {
                echo '';
            }
            break;

        case 'echo_cookies':
            header('Content-Type: application/json; charset=utf-8');
            echo json_encode($_COOKIE, JSON_PRETTY_PRINT);
            break;

        case 'echo_method':
            echo $_SERVER['REQUEST_METHOD'];
            break;

        case 'set_response_header':
            $name  = isset($_GET['name'])  ? $_GET['name']  : '';
            $value = isset($_GET['value']) ? $_GET['value'] : '';
            if ($name !== '') {
                header($name . ': ' . $value);
            }
            echo "header_set";
            break;

        case 'slow_download':
            // Streams a file slowly so progress reporting receives ~2 updates/second.
            // Params: "file" (generated.bmp, Logo.png, sample.wav, sample.flac, sample.ogg,
            // sample.jpg; default generated.bmp) and "seconds" (1-30, default 5).
            // generated.bmp creates a ~450 KB 480x320 24-bit BMP so each chunk exceeds
            // server/proxy output buffers (~8-32 KB).
            $allowed = ['Logo.png' => 'image/png', 'sample.wav' => 'audio/wav',
                        'sample.flac' => 'audio/flac', 'sample.ogg' => 'audio/ogg',
                        'sample.jpg' => 'image/jpeg'];
            $file = isset($_GET['file']) ? $_GET['file'] : 'generated.bmp';
            $seconds = isset($_GET['seconds']) ? max(1, min(30, intval($_GET['seconds']))) : 5;

            if ($file === 'generated.bmp') {
                $w = 480;
                $h = 320;
                $rowBytes = $w * 3; // 1440, already multiple of 4
                $pixelSize = $rowBytes * $h;
                $size = 54 + $pixelSize;
                $data = pack('a2Vv2VV3v2V6',
                    'BM', $size, 0, 0, 54,
                    40, $w, $h, 1, 24, 0, $pixelSize, 2835, 2835, 0, 0
                );
                // Build a smooth BGR gradient image
                $rowTemplate = '';
                for ($x = 0; $x < $w; $x++) {
                    $rowTemplate .= chr((int)($x * 255 / $w)) . "\x00\x00";
                }
                for ($y = 0; $y < $h; $y++) {
                    $g = chr((int)($y * 255 / $h));
                    $r = chr(255 - (int)($y * 255 / $h));
                    $row = '';
                    for ($x = 0; $x < $w; $x++) {
                        $row .= $rowTemplate[$x * 3] . $g . $r;
                    }
                    $data .= $row;
                }
                $contentType = 'image/bmp';
            } else {
                if (!isset($allowed[$file]) || !is_file(__DIR__ . '/' . $file)) {
                    http_response_code(404);
                    echo 'Unknown file';
                    break;
                }
                $data = file_get_contents(__DIR__ . '/' . $file);
                $size = strlen($data);
                $contentType = $allowed[$file];
            }

            @ini_set('zlib.output_compression', '0');
            @ini_set('output_buffering', 'off');
            if (function_exists('apache_setenv')) {
                @apache_setenv('no-gzip', '1');
            }

            header('Content-Type: ' . $contentType);
            header('Content-Length: ' . $size);
            header('Cache-Control: no-store');
            header('X-Accel-Buffering: no');
            while (ob_get_level() > 0) ob_end_flush();

            // 2 chunks per second (~46 KB per chunk for generated.bmp)
            $chunks = $seconds * 2;
            $chunksize = max(1, (int)ceil($size / $chunks));
            for ($pos = 0; $pos < $size; $pos += $chunksize) {
                echo substr($data, $pos, $chunksize);
                flush();
                usleep(500000);
            }
            break;

        case 'multi':
            // Multiple actions, results separated by \n---\n
            $actions = isset($_GET['actions']) ? explode(',', $_GET['actions']) : [];
            $results = [];
            foreach ($actions as $a) {
                ob_start();
                handle_action(trim($a));
                $results[] = ob_get_clean();
            }
            echo implode("\n---\n", $results);
            break;

        default:
            echo 'OK';
            break;
    }
}

handle_action($action);
