<?php
/**
 * @file
 * Contains functions to be used for Webdav file management.
 */

/**
 * WebDavClient Class.
 */
class WebDavClient {

  // Data Members.
  private $username;
  private $password;
  private $host;
  private $protocol;
  private $authentication;

  /**
   * Constructor.
   */
  public function __construct($protocol, $host, $user, $pass) {
    $this->username = $user;
    $this->password = $pass;
    $this->host = $host;
    $this->protocol = $protocol;
    $this->authentication = ($protocol == 'http') ? 'digest' : 'basic';
  }

  /**
   * To create fresh directory.
   */
  public function create_fresh_dir($dir) {
    switch ($this->protocol) {
      case 'https':
        //Commands
        $cmd_del_dir = 'curl --' . $this->authentication . ' --user "' . $this->username . ':' . $this->password . '" -X DELETE "' . $this->protocol . '://' . $this->host . '/' . $dir . '/"';
        $cmd_mk_dir = 'curl --' . $this->authentication . ' --user "' . $this->username . ':' . $this->password . '" -X MKCOL "' . $this->protocol . '://' . $this->host . '/' . $dir . '/"';

        //Deleting folder on remote server.
        $delete = shell_exec($cmd_del_dir);
        // Authorization Check.
        if ($delete && preg_match('/401/', $delete)) {
          watchdog('fila_publish', 'Error 401 Authorization problem at remote server @host.', array('@host' => $this->host), WATCHDOG_NOTICE);
          return FALSE;
        }
        if (!(!$delete || preg_match('/404/', $delete))) {
          watchdog('fila_publish', 'Error deleting country folder at remote server @host.', array('@host' => $this->host), WATCHDOG_NOTICE);
        }

        $mk_dir = shell_exec($cmd_mk_dir);
        if (!$mk_dir) {
          return TRUE;
        }
        else {
          watchdog('fila_publish', 'Error @error creating country folder at remote server @host.', array('@error'=> $mk_dir, '@host' => $this->host), WATCHDOG_ERROR);
          return FALSE;
        }
        break;

      case 'http':
        $cmd_rename_file = "curl --" . $this->authentication . " --user '" . $this->username . ":" . $this->password . "' -X MOVE --header 'Destination: " . $this->protocol . "://" . $this->host . "/" . $dir . "/" . $dir . ".html' '" . $this->protocol . "://" . $this->host . "/" . $dir . "/index.html'";
        $cmd_del_dir = 'curl --' . $this->authentication . ' --user "' . $this->username . ':' . $this->password . '" -X DELETE "' . $this->protocol . '://' . $this->host . '/' . $dir . '/"';
        $cmd_mk_dir = 'curl --' . $this->authentication . ' --user "' . $this->username . ':' . $this->password . '" -X MKCOL "' . $this->protocol . '://' . $this->host . '/' . $dir . '/"';
        // Renaming index.html
        $rename = shell_exec($cmd_rename_file);
        // Authorization Check.
        if ($rename && preg_match('/401/', $rename)) {
          watchdog('fila_publish', 'Error 401 Authorization problem at remote server @host.', array('@host' => $this->host), WATCHDOG_ERROR);
          return FALSE;
        }
        if (!($rename || preg_match('/404/', $rename))) {
          watchdog('fila_publish', 'Error renaming index file at remote server @host.', array('@host' => $this->host), WATCHDOG_NOTICE);
        }
        // Deleting folder on remote server.
        $delete = shell_exec($cmd_del_dir);
        if (!(!$delete || preg_match('/404/', $delete))) {
          watchdog('fila_publish', 'Error deleting country folder at remote server @host.', array('@host' => $this->host), WATCHDOG_NOTICE);
        }
        // Creating Directory at remote server.
        $mk_dir = shell_exec($cmd_mk_dir);
        if ((!$mk_dir) || (preg_match('/403/', $mk_dir) || preg_match('/301/', $mk_dir) || preg_match('/401/', $mk_dir) || preg_match('/405/', $mk_dir) || preg_match('/not/i', $mk_dir))) {
          watchdog('fila_publish', 'Error creating country folder at remote server @host.', array('@host' => $this->host), WATCHDOG_ERROR);
          return FALSE;
        }
        else {
          return TRUE;
        }
        break;
     }

    return FALSE;
  }

  /**
   * To upload all the contents of a folder.
   */
  public function upload_content($src, $des) {
    $d = dir($src);
    // Do this for each file in the directory.
    $count = 0;
    while ($file = $d->read()) {
      // To prevent an infinite loop.
      if ($file != "." && $file != "..") {
        // Put the files.
        $upload = "curl --" . $this->authentication . " --user '" . $this->username . ":" . $this->password . "' -T '" . $src . "/" . $file . "' '" . $this->protocol . "://" . $this->host . "/" . $des . "/'";
        $response = shell_exec($upload);
        // Check uploading of first file only.
        if ($count == 0){
          switch ($this->protocol) {
            case 'https':
              if ($response) {
                watchdog('fila_publish', 'Error @error uploading file on remote server @host.', array('@error'=> $response, '@host' => $this->host), WATCHDOG_ERROR);
                return FALSE;
              }
              break;

            case 'http':
              if ($response && (preg_match('/403/', $response) || preg_match('/301/', $response) || preg_match('/401/', $response) || preg_match('/405/', $response))) {
                watchdog('fila_publish', 'Error @error uploading file on remote server @host.', array('@error'=> $response, '@host' => $this->host), WATCHDOG_ERROR);
                return FALSE;
              }
              break;
          }
          $count++;
        }

      }
    }
    return TRUE;
  }
}
