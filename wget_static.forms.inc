<?php
/**
 * @file
 * Contains static save form.
 */

/**
 * Page callback function to get form.
 */
function wget_static_page_callback($form_type) {
  switch ($form_type) {
    case 'node':
      return drupal_get_form('wget_static_form', 'node');

    case 'path':
      return drupal_get_form('wget_static_form', 'path');

    case 'website':
      return drupal_get_form('wget_static_form', 'website');

    default:
      return drupal_not_found();
  }
}

/**
 * Implements hook_form().
 */
function wget_static_form($form, &$form_state, $form_type) {
  $form = array();

  $form['wget_static_of'] = array(
    '#type' => 'hidden',
    '#value' => $form_type,
  );

  if ($form_type != 'website') {
    // Content selection part.
    $form['wget_static'] = array(
      '#type' => 'vertical_tabs',
    );
    $form['wget_static_content'] = array(
      '#type' => 'fieldset',
      '#title' => variable_get('wget_static_content_tab_title', t('Select Content')),
      '#collapsible' => TRUE,
      '#group' => 'wget_static',
      '#description' => variable_get('wget_static_content_tab_description', t('Select contents for static HTML generation')),
    );

    $wget_static_content_form = '_wget_static_' . $form_type . '_contentform';
    $wget_static_content_form($form, $form_state);
  }

  $form['wget_static_settings'] = array(
    '#type' => 'fieldset',
    '#title' => variable_get('wget_static_settings_tab_title', t('Settings')),
    '#collapsible' => TRUE,
    '#group' => 'wget_static',
    '#description' => variable_get('wget_static_settings_tab_description', t('Settings for static HTML generation')),
  );

  _wget_static_wget_options($form, $form_state);
  _wget_static_final_settings($form, $form_state);

  return $form;
}

/**
 * Constructs Content form for node form type..
 */
function _wget_static_node_contentform(&$form, &$form_state) {
  // Access Query parameters.
  $query_params = drupal_get_query_parameters();
  $default = isset($query_params['nid']) ? $query_params['nid'] : NULL;
  $node = _wget_static_verify_nid_parameter($default);
  $form['wget_static_content']['content_type'] = array(
    '#type' => 'select',
    '#title' => t('Select Content Type'),
    '#required' => TRUE,
    '#options' => _wget_static_getcontenttypes(),
    '#ajax' => array(
      'callback' => '_wget_static_node_contentform_ajax',
      'wrapper' => 'node-contentform-data',
      'method' => 'replace',
      'effect' => 'fade',
    ),
  );
  $form['wget_static_content']['data'] = array(
    '#type' => 'fieldset',
    '#prefix' => '<div id="node-contentform-data">',
    '#suffix' => '</div>',
  );
  if ($node) {
    $form['wget_static_content']['data']['nid'] = array(
      '#type' => 'select',
      '#title' => t('Select Content'),
      '#required' => TRUE,
      '#options' => _wget_static_getcontent($node['content_type']),
    );
    // Assigning default values.
    $form['wget_static_content']['content_type']['#default_value'] = $node['content_type'];
    $form['wget_static_content']['data']['nid']['#default_value'] = $node['nid'];
    $form['wget_static']['#default_tab'] = 'edit-wget-static-settings';
  }
}

/**
 * Ajax callback for node contentform.
 */
function _wget_static_node_contentform_ajax($form, $form_state) {
  $form['wget_static_content']['data']['nid'] = array(
    '#type' => 'select',
    '#title' => t('Select Content'),
    '#required' => TRUE,
    '#options' => array_merge(
      array('0' => t('-- Please Select --')),
      _wget_static_getcontent($form_state['values']['content_type'])),
  );
  return $form['wget_static_content']['data'];
}

/**
 * Function returns array of available content types.
 */
function _wget_static_getcontenttypes() {
  $types_oa = node_type_get_types();
  foreach ($types_oa as $key => $o) {
    $types[$o->type] = $o->name;
  }
  return $types;
}

/**
 * Returns array of all the contents of provided content type.
 */
function _wget_static_getcontent($content_type) {
  $nids = db_select('node', 'n')
    ->fields('n', array('nid'))
    ->condition('n.type', $content_type)
    ->execute()
    ->fetchCol();
  foreach ($nids as $n) {
    $node = node_load($n);
    $node_array[$n] = $node->title;
  }
  return $node_array;
}

/**
 * Constructs Content form for path form type..
 */
function _wget_static_path_contentform(&$form, &$form_state) {
  // Access Query parameters.
  $query_params = drupal_get_query_parameters();
  $default = isset($query_params['url']) ? $query_params['url'] : NULL;
  $form['wget_static_content']['path'] = array(
    '#type' => 'textfield',
    '#title' => t('Enter internal path'),
    '#default_value' => $default,
    '#element_validate' => array('wget_static_path_validate'),
    '#required' => TRUE,
  );
  $form['wget_static']['#default_tab'] = 'edit-wget-static-settings';
}

/**
 * Validates Internal Path.
 */
function wget_static_path_validate($element, &$form_state, $form) {
  if (!(!empty($element['#value']) && drupal_valid_path(drupal_get_normal_path($element['#value'])) && !url_is_external($element['#value']))) {
    form_error($element, t('Please enter valid internal path.'));
  }
}
/**
 * Adds wget options form elements.
 */
function _wget_static_wget_options(&$form, &$form_state) {
  $form['wget_static_settings']['wget'] = array(
    '#type' => 'fieldset',
    '#title' => t('Wget Options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('Configure Wget for static HTML generation'),
  );

  // Directory Options.
  $form['wget_static_settings']['wget']['directory'] = array(
    '#type' => 'fieldset',
    '#title' => t('Directory Options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['wget_static_settings']['wget']['directory']['create_directory'] = array(
    '#type' => 'checkbox',
    '#title' => t('Create a hierarchy of directories when retrieving recursively.'),
    '#default_value' => FALSE,
  );
  $form['wget_static_settings']['wget']['directory']['no_host_directory'] = array(
    '#type' => 'checkbox',
    '#title' => t('Disable generation of host-prefixed directories.'),
    '#default_value' => TRUE,
  );

  // HTTP Options.
  $form['wget_static_settings']['wget']['http'] = array(
    '#type' => 'fieldset',
    '#title' => t('HTTP/HTTPS Options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['wget_static_settings']['wget']['http']['default_page'] = array(
    '#type' => 'textfield',
    '#title' => t('Default File Name'),
    '#description' => t('When it isn’t known (i.e., for URLs that end in a slash), instead of index.html.'),
  );
  $form['wget_static_settings']['wget']['http']['adjust_extension'] = array(
    '#type' => 'checkbox',
    '#title' => t('Adjust extensions automatically.'),
    '#default_value' => TRUE,
  );
  $form['wget_static_settings']['wget']['http']['cache'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable server-side cache'),
    '#default_value' => TRUE,
  );

  $form['wget_static_settings']['wget']['http']['secure_protocol'] = array(
    '#type' => 'select',
    '#title' => t('HTTPS secure Protocol'),
    '#options' => array(
      'auto' => t('auto'),
      'SSLv2' => t('SSLv2'),
      'SSLv3' => t('SSLv3'),
      'TLSv1' => t('TLSv1'),
      'TLSv1_1' => t('TLSv1_1'),
      'TLSv1_2' => t('TLSv1_2'),
      'PFS' => t('PFS'),
    ),
    '#default_value' => 'auto',
  );

  $form['wget_static_settings']['wget']['http']['httpsonly'] = array(
    '#type' => 'checkbox',
    '#title' => t('HTTPS Links only'),
    '#description' => t('When Enabled in recursive mode, only HTTPS links will be followed.'),
    '#default_value' => FALSE,
  );

  // Recursive Retrieval Options.
  $form['wget_static_settings']['wget']['rec'] = array(
    '#type' => 'fieldset',
    '#title' => t('Recursive Retrieval Options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['wget_static_settings']['wget']['rec']['enable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable recursive retrieving'),
    '#default_value' => TRUE,
  );
  $form['wget_static_settings']['wget']['rec']['depth'] = array(
    '#type' => 'textfield',
    '#title' => t('Specify recursion maximum depth level depth'),
    '#description' => t('Enter depth level, -1 for maximum depth'),
    '#maxlength' => 2,
    '#element_validate' => array('element_validate_integer'),
    '#default_value' => 5,
  );
  $form['wget_static_settings']['wget']['rec']['convert_links'] = array(
    '#type' => 'checkbox',
    '#title' => t('Convert Links'),
    '#description' => t('After the download is complete, convert the links in the document to make them suitable for local viewing.'),
    '#default_value' => TRUE,
  );
  $form['wget_static_settings']['wget']['rec']['page_requisites'] = array(
    '#type' => 'checkbox',
    '#title' => t('Page Requisites'),
    '#description' => t('Downloads all the files that are necessary to properly display a given HTML page.'),
    '#default_value' => TRUE,
  );

  // Accept/Reject List.
  $form['wget_static_settings']['wget']['accept'] = array(
    '#type' => 'fieldset',
    '#title' => t('Accept/Reject Options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['wget_static_settings']['wget']['accept']['domains'] = array(
    '#type' => 'select',
    '#title' => t('Domains Options'),
    '#options' => array(
      0 => t('Do nothing'),
      'accept' => t('Follow these domains only'),
      'reject' => t('Exclude these domains'),
    ),
    '#default_value' => 0,
    '#description' => t('Set domains to be followed or excluded.'),
  );
  $form['wget_static_settings']['wget']['accept']['domainsaccept'] = array(
    '#type' => 'textfield',
    '#title' => t('Enter domains to be followed'),
    '#description' => t('Use comma for multiple domains. Ex: example.com,drupal.org'),
    '#states' => array(
      'visible' => array(
        ':input[name="domains"]' => array('value' => 'accept'),
      ),
    ),
  );
  $form['wget_static_settings']['wget']['accept']['domainsreject'] = array(
    '#type' => 'textfield',
    '#title' => t('Enter domains to be excluded'),
    '#description' => t('Use comma for multiple domains. Ex: example.com,drupal.org'),
    '#states' => array(
      'visible' => array(
        ':input[name="domains"]' => array('value' => 'reject'),
      ),
    ),
  );
  $form['wget_static_settings']['wget']['accept']['tags'] = array(
    '#type' => 'select',
    '#title' => t('Tags Options'),
    '#options' => array(
      0 => t('Do nothing'),
      'accept' => t('Consider these tags only'),
      'reject' => t('Exclude these tags'),
    ),
    '#default_value' => 0,
    '#description' => t('Set tags to be considered or excluded.'),
  );
  $form['wget_static_settings']['wget']['accept']['tagsaccept'] = array(
    '#type' => 'textfield',
    '#title' => t('Enter tags to be considered'),
    '#description' => t('Use comma for multiple tags. Ex: a,area'),
    '#states' => array(
      'visible' => array(
        ':input[name="tags"]' => array('value' => 'accept'),
      ),
    ),
  );
  $form['wget_static_settings']['wget']['accept']['tagsreject'] = array(
    '#type' => 'textfield',
    '#title' => t('Enter tags to be excluded'),
    '#description' => t('Use comma for multiple tags. Ex: a,area'),
    '#states' => array(
      'visible' => array(
        ':input[name="tags"]' => array('value' => 'reject'),
      ),
    ),
  );
  $form['wget_static_settings']['wget']['accept']['files'] = array(
    '#type' => 'select',
    '#title' => t('Files Options'),
    '#options' => array(
      0 => t('Do nothing'),
      'accept' => t('Consider these files only'),
      'reject' => t('Exclude these files'),
    ),
    '#default_value' => 0,
    '#description' => t('Set files to be considered or excluded.'),
  );
  $form['wget_static_settings']['wget']['accept']['filesaccept'] = array(
    '#type' => 'textfield',
    '#title' => t('Enter file extensions to be considered'),
    '#description' => t('Use comma for multiple extensions. Ex: png,mp3'),
    '#states' => array(
      'visible' => array(
        ':input[name="files"]' => array('value' => 'accept'),
      ),
    ),
  );
  $form['wget_static_settings']['wget']['accept']['filesreject'] = array(
    '#type' => 'textfield',
    '#title' => t('Enter file extensions to be excluded'),
    '#description' => t('Use comma for multiple extensions. Ex: png,mp3'),
    '#states' => array(
      'visible' => array(
        ':input[name="files"]' => array('value' => 'reject'),
      ),
    ),
  );
  $form['wget_static_settings']['wget']['accept']['relative_links_only'] = array(
    '#type' => 'checkbox',
    '#title' => t('Follow relative links only.'),
    '#description' => t('Useful for retrieving a specific home page without any distractions, not even those from the same hosts.'),
    '#default_value' => FALSE,
  );
  $form['wget_static_settings']['wget']['accept']['robots'] = array(
    '#type' => 'checkbox',
    '#title' => t('Consider Robots'),
    '#description' => t('Specify whether the robots is to be respected by Wget.'),
    '#default_value' => TRUE,
  );
  if (user_access('wget use supercmd')) {
    $form['wget_static_settings']['wget']['supercmd'] = array(
      '#type' => 'textfield',
      '#title' => t('Specify wget options directly other than provided ones.'),
      '#description' => t('This input will directly appended to the wget command responsible for generation of static HTML'),
    );
  }
}

/**
 * Adds save/download settings for elements.
 */
function _wget_static_final_settings(&$form, &$form_state) {
  $form['wget_static_settings']['final'] = array(
    '#type' => 'select',
    '#title' => t('Save generated static HTML'),
    '#options' => array(
      'none' => t('What to do?'),
      'download' => t('Download'),
      'savetoftp' => t('Save to FTP'),
    ),
  );
  $form['wget_static_settings']['download_file'] = array(
    '#type' => 'textfield',
    '#maxlength' => 30,
    '#title' => t('Name for compressed file'),
    '#states' => array(
      'visible' => array(
        ':input[name="final"]' => array('value' => 'download'),
      ),
    ),
  );
  $form['wget_static_settings']['download'] = array(
    '#type' => 'submit',
    '#value' => t('Download'),
    '#states' => array(
      'visible' => array(
        ':input[name="final"]' => array('value' => 'download'),
      ),
    ),
  );
  $form['wget_static_settings']['ftp'] = array(
    '#type' => 'fieldset',
    '#title' => t('FTP settings'),
    '#states' => array(
      'visible' => array(
        ':input[name="final"]' => array('value' => 'savetoftp'),
      ),
    ),
  );
  $form['wget_static_settings']['ftp']['host'] = array(
    '#type' => 'textfield',
    '#title' => t('FTP Server Location'),
    '#description' => t('Please exclude protocol ftp:// at beginning and trailing slashes (/) at the end.'),
    '#element_validate' => array('_wget_static_validate_host'),
  );
  $form['wget_static_settings']['ftp']['username'] = array(
    '#type' => 'textfield',
    '#title' => t('Username'),
  );
  $form['wget_static_settings']['ftp']['password'] = array(
    '#type' => 'textfield',
    '#title' => t('Password'),
  );
  $form['wget_static_settings']['ftp']['location'] = array(
    '#type' => 'textfield',
    '#title' => t('Remote Folder Name'),
    '#description' => t('Folder name in which static html has to be saved on remote server. Any existing directory of the same name would be removed.'),
  );
  $form['wget_static_settings']['ftp']['compressed_file'] = array(
    '#type' => 'checkbox',
    '#title' => t('Send as Compressed File on FTP Server'),
    '#default_value' => FALSE,
    '#description' => t('Compressed files would be sent faster.'),
  );
  $form['wget_static_settings']['ftp']['ftp_filename'] = array(
    '#type' => 'textfield',
    '#maxlength' => 30,
    '#title' => t('Name for compressed file'),
    '#states' => array(
      'visible' => array(
        ':input[name="compressed_file"]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['wget_static_settings']['ftp']['savetoftp'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
}

/**
 * Validates FTP host name.
 */
function _wget_static_validate_host($element, &$form_state, $form) {
  if (!empty($element['#value'])) {
    if (preg_match('/http:/', $element['#value']) || preg_match('/ftp:/', $element['#value']) || preg_match('/https:/', $element['#value']) || preg_match('/ftps:/', $element['#value'])) {
      form_error($element, t('Please ensure Host field does not contain protocol like http://, https:// or ftp://, ftps://.'));
    }
    if (preg_match('#/$#', $element['#value'])) {
      form_error($element, t('Please ensure Host field does not end with "/"'));
    }
    if (!@ftp_connect($element['#value'])) {
      form_error($element, t('Failed to connect to FTP server'));
    }
  }
  else {
    if ($form_state['values']['final'] == 'savetoftp') {
      form_error($element, t('FTP host field is requorired'));
    }
  }
}

/**
 * Function verifies default variable.
 */
function _wget_static_verify_nid_parameter($default) {
  if (empty($default)) {
    return FALSE;
  }
  if (is_numeric($default)) {
    $node = node_load($default);
    if ($node) {
      return $default_node = array(
        'nid' => $node->nid,
        'title' => $node->title,
        'content_type' => $node->type,
      );
    }
  }
}

/**
 * Submit handler of wget static form.
 */
function wget_static_form_submit(&$form, &$form_state) {
  $temp_dir = file_directory_temp();
  $timestamp = time();
  $wget_dir = 'wget/' . $form_state['values']['wget_static_of'] . '/' . $timestamp;

  // Create static html at temporary directory.
  if (!_wget_static_generate_static_html($temp_dir, $wget_dir, $form_state)) {
    drupal_set_message(t('Error creating temporary static html.'), 'error', FALSE);
    return;
  }

  switch ($form_state['values']['final']) {
    case 'download':
      $download_url = _wget_static_create_archive($temp_dir, $wget_dir, $form_state['values']['download_file'], $timestamp, TRUE);
      if (!$download_url) {
        file_unmanaged_delete_recursive($temp_dir . "/wget");
        return;
      }
      break;

    case 'savetoftp':
      if (!_wget_static_savetoftp($temp_dir, $wget_dir, $form_state, $timestamp)) {
        file_unmanaged_delete_recursive($temp_dir . "/wget");
        return;
      }
      break;
  }

  if (variable_get('wget_static_success', 0)) {
    $form_state['redirect'] = url(variable_get('wget_static_success_redirect', '<front>'), array('absolute' => TRUE));
  }
  else {
    $form_state['rebuild'] = TRUE;
    drupal_set_message(variable_get('wget_static_success_message', 'Operation Successfully Completed'), 'status', FALSE);
  }
}

/**
 * Returns array of wget options.
 */
function _wget_static_build_options($form_state) {
  $wget = array(
    'no_dir' => array(
      'use' => ($form_state['values']['create_directory']) ? FALSE : TRUE,
      'cmd' => '-nd',
    ),
    'force_dir' => array(
      'use' => ($form_state['values']['create_directory']) ? TRUE : FALSE,
      'cmd' => '-x',
    ),
    'no_host_dir' => array(
      'use' => ($form_state['values']['no_host_directory']) ? TRUE : FALSE,
      'cmd' => '-nH',
    ),
    'default_page' => array(
      'use' => ($form_state['values']['default_page']) ? TRUE : FALSE,
      'cmd' => '--default-page=' . preg_replace("/[^\p{L}\p{N}\.\-\_]/", "", trim($form_state['values']['default_page'])),
    ),
    'adjust_extension' => array(
      'use' => ($form_state['values']['adjust_extension']) ? TRUE : FALSE,
      'cmd' => '-E',
    ),
    'nocache' => array(
      'use' => ($form_state['values']['cache']) ? FALSE : TRUE,
      'cmd' => '--no-cache',
    ),
    'httpssecureprotocol' => array(
      'use' => ($form_state['values']['secure_protocol'] == 'auto') ? FALSE : TRUE,
      'cmd' => '--secure-protocol=' . $form_state['values']['secure_protocol'],
    ),
    'httpsonly' => array(
      'use' => ($form_state['values']['httpsonly']) ? TRUE : FALSE,
      'cmd' => '--https-only',
    ),
    'recretrieval' => array(
      'use' => ($form_state['values']['enable']) ? TRUE : FALSE,
      'cmd' => '-r',
    ),
    'depthlevel' => array(
      'use' => (($form_state['values']['enable'] == TRUE) && $form_state['values']['depth'] != '5') ? TRUE : FALSE,
      'cmd' => '--level=' . $form_state['values']['depth'],
    ),
    'convertlinks' => array(
      'use' => ($form_state['values']['convert_links']) ? TRUE : FALSE,
      'cmd' => '-k',
    ),
    'page_requisites' => array(
      'use' => ($form_state['values']['page_requisites']) ? TRUE : FALSE,
      'cmd' => '-p',
    ),
    'followdomains' => array(
      'use' => ($form_state['values']['domains'] == 'accept') ? TRUE : FALSE,
      'cmd' => '--domains=' . preg_replace('/\ /', '', $form_state['values']['domainsaccept']),
    ),
    'excludedomains' => array(
      'use' => ($form_state['values']['domains'] == 'reject') ? TRUE : FALSE,
      'cmd' => '--exclude-domains ' . preg_replace('/\ /', '', $form_state['values']['domainsreject']),
    ),
    'followtags' => array(
      'use' => ($form_state['values']['tags'] == 'accept') ? TRUE : FALSE,
      'cmd' => '--follow-tags=' . preg_replace('/[^\p{L}\,]/', '', $form_state['values']['tagsaccept']),
    ),
    'ignoretags' => array(
      'use' => ($form_state['values']['tags'] == 'reject') ? TRUE : FALSE,
      'cmd' => '--ignore-tags=' . preg_replace('/[^\p{L}\,]/', '', $form_state['values']['tagsreject']),
    ),
    'acceptfiles' => array(
      'use' => ($form_state['values']['files'] == 'accept') ? TRUE : FALSE,
      'cmd' => '-A ' . preg_replace("/[^\p{L}\p{N}\,]/", "", trim($form_state['values']['filesaccept'])),
    ),
    'rejectlist' => array(
      'use' => ($form_state['values']['files'] == 'reject') ? TRUE : FALSE,
      'cmd' => '-A ' . preg_replace("/[^\p{L}\p{N}\,]/", "", trim($form_state['values']['filesreject'])),
    ),
    'followrellinksonly' => array(
      'use' => ($form_state['values']['relative_links_only']) ? TRUE : FALSE,
      'cmd' => '-L',
    ),
    'robotsoff' => array(
      'use' => ($form_state['values']['robots']) ? FALSE : TRUE,
      'cmd' => '-e robots=off',
    ),
    'supercmd' => array(
      'use' => (isset($form_state['values']['supercmd']) && !empty($form_state['values']['supercmd'])) ? TRUE : FALSE,
      'cmd' => isset($form_state['values']['supercmd']) ? $form_state['values']['supercmd'] : '',
    ),
  );
  return $wget;
}

/**
 * Builds wget url.
 */
function _wget_static_build_url($values) {
  switch ($values['wget_static_of']) {
    case 'node':
      return url('node/' . $values['nid'], array('absolute' => TRUE));

    case 'path':
      return url($values['path'], array('absolute' => TRUE));

    case 'website':
      return url('', array('absolute' => TRUE));
  }
}

/**
 * Builds wget final command.
 */
function _wget_static_build_command($options, $url, $temp_dir, $wget_dir) {
  $wget = trim(shell_exec("which wget"));
  $wget = ($wget) ? $wget : variable_get('wget_static_command', '/usr/local/bin/wget');
  $cmd = $wget . " ";
  foreach ($options as $key => $option) {
    if ($option['use']) {
      $cmd .= $option['cmd'] . " ";
    }
  }
  $cmd .= "-o " . $temp_dir . "/wget.log -P" . $temp_dir . "/" . $wget_dir . "/ " . $url;
  return $cmd;
}

/**
 * Generates static html.
 */
function _wget_static_generate_static_html($temp_dir, $wget_dir, $form_state) {
  $wget_options = _wget_static_build_options($form_state);
  $wget_url = _wget_static_build_url($form_state['values']);
  $wget_cmd = _wget_static_build_command($wget_options, $wget_url, $temp_dir, $wget_dir);
  file_unmanaged_delete_recursive($temp_dir . "/wget");
  // Check for debug mode.
  if (variable_get('wget_static_enable_wget_log', FALSE)) {
    $log = shell_exec($wget_cmd . " 2>&1");
    watchdog('wget_static', '<pre> @log </pre>', array('@log' => $log), WATCHDOG_NOTICE, 'link');
    watchdog('wget_static', 'wget command built: @cmd', array('@cmd' => $wget_cmd), WATCHDOG_NOTICE, 'link');
  }
  else {
    shell_exec($wget_cmd);
  }
  return TRUE;
}

/**
 * Generates zip archive.
 */
function _wget_static_create_archive($temp_dir, $wget_dir, $filename, $timestamp, $download = FALSE) {
  // Load class.
  include_once "zip/recursiveZip.php";
  $zip = new RecursiveZip();
  $filename = preg_replace('/[^\p{L}\p{N}\-\_]/', '', $filename);
  $filename = ($filename) ? $filename . '.zip' : $timestamp . '.zip';
  $filepath = $zip->compress($temp_dir . "/" . $wget_dir, $temp_dir . "/wget/", $filename);
  if (!$filepath) {
    drupal_set_message(t('Unable to compress'), 'error', FALSE);
    return FALSE;
  }
  if ($download) {
    drupal_add_http_header('Content-disposition', 'attachment; filename=' . $filename);
    readfile($filepath);
    drupal_exit();
  }
  else {
    return $filepath;
  }
}

/**
 * Uses ftp library to upload content on remote ftp server.
 */
function _wget_static_savetoftp($temp_dir, $wget_dir, $form_state, $timestamp) {
  // *** Include the class.
  include_once 'ftp/ftp_class.php';

  // *** Create the FTP object.
  $ftpobj = new FTPClient();
  // *** Connect.
  if (!$ftpobj->connect($form_state['values']['host'], $form_state['values']['username'], $form_state['values']['password'], TRUE)) {
    drupal_set_message(t('Failed to connect to FTP server'), 'error', FALSE);
    return FALSE;
  }
  // *** Make fresh directory.
  $folder = preg_replace("/[^\p{L}\p{N}\-\_]/", "", $form_state['values']['location']);
  if ($folder) {
    if (!$ftpobj->makeDir($folder)) {
      drupal_set_message(t('Unable to create directory at the Remote FTP server'), 'error', FALSE);
      return FALSE;
    }
  }

  if ($form_state['values']['compressed_file']) {
    $filepath = _wget_static_create_archive($temp_dir, $wget_dir, $form_state['values']['download_file'], $timestamp, FALSE);
    if (!$filepath) {
      return FALSE;
    }
    $filename = preg_replace('/[^\p{L}\p{N}\-\_]/', '', $form_state['values']['ftp_filename']);
    $filename = ($filename) ? $filename : $timestamp;
    if (!$ftpobj->uploadFile($filepath, $folder . '/' . $filename . '.zip')) {
      drupal_set_message(t('Unable to upload compressed at the Remote FTP server'), 'error', FALSE);
      return FALSE;
    }
  }
  else {
    // *** Upload static content complete directory on remote server.
    if (!$ftpobj->ftp_putAll($temp_dir . "/" . $wget_dir, $folder)) {
      drupal_set_message(t('Unable to upload files at the Remote FTP server'), 'error', FALSE);
      return FALSE;
    }
  }

  return TRUE;
}