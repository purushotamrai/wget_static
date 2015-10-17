<?php
/**
 * @file
 * FTP class.
 */

/**
 * FTP Client Class.
 */
class FTPClient {
  private $connectionId;
  private $loginOk = FALSE;
  private $messageArray = array();

  /**
   * Constructor.
   */
  public function __construct() {}

  /**
   * Deconstructor.
   */
  public function __destruct() {
    if ($this->connectionId) {
      ftp_close($this->connectionId);
    }
  }

  /**
   * Message logger.
   */
  private function logMessage($message) {
    $this->messageArray[] = $message;
  }

  /**
   * Returns Logged messages.
   */
  public function getMessages() {
    return $this->messageArray;
  }

  /**
   * For establishing connection.
   */
  public function connect($server, $ftpUser, $ftpPassword, $isPassive = FALSE) {
    // *** Set up basic connection.
    $this->connectionId = ftp_connect($server);
    // *** Login with username and password.
    $loginResult = ftp_login($this->connectionId, $ftpUser, $ftpPassword);
    // *** Sets passive mode on/off (default off).
    ftp_pasv($this->connectionId, $isPassive);
    // *** Check connection.
    if ((!$this->connectionId) || (!$loginResult)) {
      $this->logMessage('FTP connection has failed!');
      $this->logMessage('Attempted to connect to ' . $server . ' for user ' . $ftpUser, TRUE);
      return FALSE;
    }
    else {
      $this->logMessage('Connected to ' . $server . ', for user ' . $ftpUser);
      $this->loginOk = TRUE;
      return TRUE;
    }
  }

  /**
   * For creating fresh directory.
   */
  public function makeDir($directory) {
    // Removing existing directory.
    if (ftp_directory_exists($this->connectionId, $directory)) {
      if (!ftp_rmdirr($this->connectionId, $directory)) {
        $this->logMessage('Directory "' . $directory . '"could not be deleted on remote server.');
        return FALSE;
      }
    }

    // *** If creating a directory is successful.
    if (ftp_mkdir($this->connectionId, $directory)) {
      $this->logMessage('Directory "' . $directory . '" created successfully');
      return TRUE;
    }
    else {
      // *** ...Else, FAIL.
      $this->logMessage('Failed creating directory "' . $directory . '"');
      return FALSE;
    }

  }

  /**
   * To upload single file.
   */
  public function uploadFile($fileFrom, $fileTo) {
    // *** Set the transfer mode.
    $asciiArray = array('txt', 'csv', 'html', 'css', 'zip');
    $temp_array = explode('.', $fileFrom);
    $extension = end($temp_array);
    if (in_array($extension, $asciiArray)) {
      $mode = FTP_ASCII;
    }
    else {
      $mode = FTP_BINARY;
    }

    // Turn passive mode on.
    ftp_pasv($this->connectionId, TRUE);

    // *** Upload the file.
    $upload = ftp_put($this->connectionId, $fileTo, $fileFrom, $mode);

    // *** Check upload status.
    if (!$upload) {
      $this->logMessage('FTP upload has failed!');
      return FALSE;
    }
    else {
      $this->logMessage('Uploaded "' . $fileFrom . '" as "' . $fileTo);
      return TRUE;
    }
  }

  /**
   * Uploads whole directory to remote server.
   */
  public function ftp_putAll($src_dir, $dst_dir) {
    $conn_id = $this->connectionId;
    $d = dir($src_dir);
    while ($file = $d->read()) {
      // Do this for each file in the directory.
      if ($file != "." && $file != "..") {
        // To prevent an infinite loop.
        if (is_dir($src_dir . "/" . $file)) {
          // Do the following if it is a directory.
          if (!@ftp_chdir($conn_id, $dst_dir . "/" . $file)) {
            // Create directories that do not yet exist.
            ftp_mkdir($conn_id, $dst_dir . "/" . $file);
          }
          // Recursive part.
          ftp_putAll($conn_id, $src_dir . "/" . $file, $dst_dir . "/" . $file);
        }
        else {
          // Put the files.
          $upload = ftp_put($this->connectionId, $dst_dir . "/" . $file, $src_dir . "/" . $file, FTP_BINARY);
        }
      }
    }
    $d->close();
    return TRUE;
  }

}

/**
 * Verifies if directory exists or not.
 */
function ftp_directory_exists($ftp, $dir) {
  // Get the current working directory.
  $origin = ftp_pwd($ftp);

  // Attempt to change directory, suppress errors.
  if (@ftp_chdir($ftp, $dir)) {
    // If the directory exists, set back to origin.
    ftp_chdir($ftp, $origin);
    return TRUE;
  }

  // Directory does not exist.
  return FALSE;
}

/**
 * Recursively delete the files in a directory via FTP.
 */
function ftp_rmdirr($ftp_stream, $directory) {
  // Sanity check.
  if (!is_resource($ftp_stream) || get_resource_type($ftp_stream) !== 'FTP Buffer') {
    return FALSE;
  }

  // Init.
  $i             = 0;
  $files         = array();
  $folders       = array();
  $statusnext    = FALSE;
  $currentfolder = $directory;

  // Get raw file listing.
  $list = ftp_rawlist($ftp_stream, $directory, TRUE);

  // Iterate listing.
  foreach ($list as $current) {
    // An empty element means the next element will be the new folder.
    if (empty($current)) {
      $statusnext = TRUE;
      continue;
    }

    // Save the current folder.
    if ($statusnext === TRUE) {
      $currentfolder = substr($current, 0, -1);
      $statusnext = FALSE;
      continue;
    }

    // Split the data into chunks.
    $split = preg_split('[ ]', $current, 9, PREG_SPLIT_NO_EMPTY);
    $entry = $split[8];
    $isdir = ($split[0]{0} === 'd') ? TRUE : FALSE;

    // Skip pointers.
    if ($entry === '.' || $entry === '..') {
      continue;
    }

    // Build the file and folder list.
    if ($isdir === TRUE) {
      $folders[] = $currentfolder . '/' . $entry;
    }
    else {
      $files[] = $currentfolder . '/' . $entry;
    }
  }

  // Delete all the files.
  foreach ($files as $file) {
    ftp_delete($ftp_stream, $file);
  }

  // Reverse sort the folders so the deepest directories are unset first.
  rsort($folders);
  // Delete all the directories.
  foreach ($folders as $folder) {
    ftp_rmdir($ftp_stream, $folder);
  }

  // Delete the final folder and return its status.
  return ftp_rmdir($ftp_stream, $directory);
}
