<?php
/**
 * Gorgon HTTP Test Endpoint
 * Upload to: https://darkgaze.org/testing/gorgontest.php
 *
 * Supports the following query actions via GET parameter "action":
 *
 *   echo_headers   - Returns all received request headers as JSON
 *   echo_post      - Returns the raw POST body
 *   echo_all       - Returns JSON with both headers and POST body
 *   set_cookie     - Sets cookies specified in query params "name" and "value"
 *                    Optionally accepts "path", "domain", "secure", "httponly",
 *                    "samesite", "maxage" query params for cookie attributes
 *   check_cookie   - Returns the value of cookie specified by "name" query param
 *   echo_cookies   - Returns all received cookies as JSON
 *   echo_method    - Returns the HTTP method used
 *   set_response_header - Sets a response header from "name" and "value" query params
 *   multi          - Performs multiple actions separated by comma in "actions" param
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
