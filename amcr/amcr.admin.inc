<?php

/**
 * @file
 * Administration page callbacks for the Aggregator module.
 */



/**
 * Displays the reports list.
 *
 * @return
 *   A HTML-formatted string with list of reports.
 */
function amcr_report_list() {
  $result = db_query('SELECT rd.id, rd.name, rd.last_update, rd.update_frequency '.
    'FROM {amcr_report_description} rd ORDER BY rd.name');

  $output = '<h3>' . t('Report list') . '</h3>';

  $header = array(t('ID'), t('Name'), t('Last update'), t('Next update'),
    array('data' => t('Operations'), 'colspan' => '3'));

  $rows = array();
  foreach ($result as $report) {
    $rows[] = array(
      $report->id,
      $report->name,
      ($report->last_update ? format_date($report->last_update,'long') : t('never')),
      ($report->update_frequency ? format_date($report->last_update + $report->update_frequency,'long') : t('never')),
      l(t('edit'), "admin/config/services/amcr/edit/$report->id"),
      l(t('remove'), "admin/config/services/amcr/remove/$report->id"),
      l(t('run now'), "admin/config/services/amcr/update/$report->id"),
    );
  }
  $output .= theme('table', array('header' => $header, 'rows' => $rows,
    'empty' => t('No reports available. <a href="@link">Add report</a>.',
      array('@link' => url('admin/config/services/amcr/add/')))));

  return $output;
}

/**
 * Form constructor for adding and editing report config.
 *
 * @param $report
 *   (optional) If editing a report, the report to edit as a PHP stdClass value; if
 *   adding a new report, NULL. Defaults to NULL.
 *
 * @ingroup forms
 * @see amcr_form_report_validate()
 * @see amcr_form_report_submit()
 */

function amcr_report_form($form, &$form_state, stdClass $report = NULL) {

  $methods = array('ranked' => 'Ranked report',
    'trended' => 'Trended report',
    'overtime' => 'Overtime report',
    'realtime' => 'Real time report',
  );

  $metrics = array('pageviews' => 'Page views',
    'uniquevisitors' => 'Unique visitors',
  );

  $form['name'] = array('#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => isset($report->name) ? $report->name : '',
    '#maxlength' => 255,
    '#description' => t('The name of the report. It is shown in list of reports.'),
    '#required' => TRUE,
  );
  $form['description'] = array('#type' => 'textarea',
    '#title' => t('Description'),
    '#default_value' => isset($report->description) ? $report->description : '',
    '#maxlength' => NULL,
    '#description' => t('Report Description. It is shown or report details page'),
    '#required' => TRUE,
  );
  $form['report_suite_id'] = array('#type' => 'textfield',
    '#title' => t('Report suite id'),
    '#default_value' => isset($report->report_suite_id) ? $report->report_suite_id : '',
    '#maxlength' => NULL,
    '#description' => t('Id of omniture report suite'),
    '#required' => TRUE,
  );
  $form['method'] = array('#type' => 'select',
    '#title' => t('Report type'),
    '#default_value' => isset($report->method) ? $report->method : 'ranked',
    '#options' => $methods,
  );


  /* Elements list */

  $result = db_query('SELECT id, element_id, weight FROM {amcr_report_definition_element} ORDER BY weight ASC');

  if (isset($form_state['elements_count'])) {
    $elements_count = $form_state['elements_count'];
  }
  else {
    $elements_count = max(2, empty($report->elements->items) ? 2 : count($report->elements->items));
  }

  $form['elements']['#title'] = t('Elements');
  $form['elements']['#tree'] = true;
  $form['elements']['#description'] = t('A list of elements that breaks down '.
    '(organizes) the metrics data in the report');
  $form['elements']['#theme'] = 'amcr_element_form';
  $form['elements']['#prefix'] = '<div id="amcr-elements-wrapper">';
  $form['elements']['#suffix'] = '</div>';

  $delta = 0;
  $weight = 0;
  foreach ($result as $item) {
    $form['elements']['items'][$item->id] = _amcr_element_form($item);
    $delta++;
    if($item->weight > $weight) {
      $weight = $item->weight;
    }
  }

  $existing_delta = $delta;
  for ($delta; $delta < $elements_count; $delta++) {
    $key = 'new:' . ($delta - $existing_delta);
    $weight++;
    $item = new stdClass();
    $item->weight = $weight;
    $form['elements']['items'][$key] = _amcr_element_form($item);
  }

  $form['elements']['add'] = array(
    '#type' => 'submit',
    '#value' => t('Add element'),
    '#attributes' => array(
      'title' => t("If the amount of boxes above isn't enough, click here to add more elements."),
    ),
    '#weight' => 1,
    '#submit' => array('amcr_add_element_submit'),
    '#ajax' => array(
      'callback' => 'amcr_element_js',
      'wrapper' => 'amcr-elements-wrapper',
      'effect' => 'fade',
    ),
    '#limit_validation_errors' => array(array('elements')),
  );


  /* End of elements list* /

  /* Metrics list */
  $result = db_query('SELECT id, metric_id, weight FROM {amcr_report_definition_metric} ORDER BY weight ASC');

  if (isset($form_state['metrics_count'])) {
    $metrics_count = $form_state['metrics_count'];
  }
  else {
    $metrics_count = max(2, empty($report->metrics->items) ? 2 : count($report->metrics->items));
  }

  $form['metrics']['#title'] = t('Metrics');
  $form['metrics']['#tree'] = true;
  $form['metrics']['#theme'] = 'amcr_metric_form';
  $form['metrics']['#prefix'] = '<div id="amcr-metrics-wrapper">';
  $form['metrics']['#suffix'] = '</div>';

  $delta = 0;
  $weight = 0;
  foreach ($result as $item) {
    $form['metrics']['items'][$item->id] = _amcr_metric_form($item);
    $delta++;
    if($item->weight > $weight) {
      $weight = $item->weight;
    }
  }

  $existing_delta = $delta;
  for ($delta; $delta < $metrics_count; $delta++) {
    $key = 'new:' . ($delta - $existing_delta);
    $weight++;
    $item = new stdClass();
    $item->weight = $weight;
    $form['metrics']['items'][$key] = _amcr_metric_form($item);
  }

  $form['metrics']['add'] = array(
    '#type' => 'submit',
    '#value' => t('Add metric'),
    '#attributes' => array(
      'title' => t("If the amount of boxes above isn't enough, click here to add more metrics."),
    ),
    '#weight' => 1,
    '#submit' => array('amcr_add_metric_submit'),
    '#ajax' => array(
      'callback' => 'amcr_metric_js',
      'wrapper' => 'amcr-metrics-wrapper',
      'effect' => 'fade',
    ),
    '#limit_validation_errors' => array(array('metrics')),
  );
  /* End of metrics list */





  $form['date_from'] = array('#type' => 'date',
    '#title' => t('Date from'),
    '#default_value' => isset($report->date_from) ? $report->date_from : '',
    '#maxlength' => NULL,
    '#description' => t('Data is analysed since \'Date from \' till \'Date to\'. It is '.
      'used if \'Period\' is not specified '),
    '#required' => FALSE,
  );
  $form['date_to'] = array('#type' => 'date',
    '#title' => t('Date to'),
    '#default_value' => isset($report->date_to) ? $report->date_to : '',
    '#maxlength' => NULL,
    '#description' => t('Data is analysed since \'Date from \' till \'Date to\'. It is '.
      'used if \'Period\' is not specified '),
    '#required' => FALSE,
  );
  $form['period'] = array('#type' => 'textfield',
    '#title' => t('Period'),
    '#default_value' => isset($report->period) ? $report->period / 3600 : '',
    '#maxlength' => NULL,
    '#size' => 5,
    '#description' => t('Period in hours for getting data. It is counted for the' .
      ' moment report execution.'),
    '#required' => FALSE,
  );
  $form['order_by'] = array('#type' => 'select',
    '#title' => t('Order by'),
    '#default_value' => isset($report->order_by) ? $report->order_by : '',
    '#options' => $metrics,
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  if (!empty($report->id)) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
    );
    $form['id'] = array(
      '#type' => 'hidden',
      '#value' => $report->id,
    );
  }

  return $form;
}


/**
 * Ajax callback in response to new element added to the form.
 *
 * This returns the new page content to replace the page content made obsolete
 * by the form submission.
 *
 * @see amcr_add_element_submit()
 */
function amcr_element_js($form, $form_state) {
  return $form['elements'];
}

/**
 * Ajax callback in response to new metric added to the form.
 *
 * This returns the new page content to replace the page content made obsolete
 * by the form submission.
 *
 * @see amcr_add_metric_submit()
 */
function amcr_metric_js($form, $form_state) {
  return $form['metrics'];
}

/**
 * Implements 'Add element' submit handler
 */
function amcr_add_element_submit($form, &$form_state) {
  if ($form_state['values']['elements']['add']) {
    $n = $_GET['q'] == 'system/ajax' ? 1 : 5;
    $form_state['elements_count'] = count($form_state['values']['elements']['items']) + $n;
  }

  unset($form_state['input']['elements']);
  $form_state['rebuild'] = TRUE;
}


/**
 * Implements 'Add metric' submit handler
 */
function amcr_add_metric_submit($form, &$form_state) {
  if ($form_state['values']['metrics']['add']) {
    $n = $_GET['q'] == 'system/ajax' ? 1 : 5;
    $form_state['metrics_count'] = count($form_state['values']['metrics']['items']) + $n;
  }

  unset($form_state['input']['metrics']);
  $form_state['rebuild'] = TRUE;
}

/*
 * Generates form structure for element item
 *
 * @param $item
 *  report element
 */
function _amcr_element_form($item = NULL) {
  $form = array(
    'id' => array(
      '#type' => 'textfield',
      '#default_value' => isset($item->element_id) ? $item->element_id : '',
      '#maxlength' => 100,
    ),
    'weight' => array(
      '#type' => 'weight',
      '#title' => t('Weight'),
      '#default_value' => $item->weight,
      '#title_display' => 'invisible',
    ),
  );

  return $form;
}


/*
 * Generates form structure for metric item
 *
 * @param $item
 *  report metric
 */
function _amcr_metric_form($item = NULL) {
  $form = array(
    '#tree' => true,
    'id' => array(
      '#type' => 'textfield',
      '#default_value' => isset($item->metric_id) ? $item->metric_id : '',
      '#maxlength' => 100,
      '#tree' => true,
    ),
    'weight' => array(
      '#type' => 'weight',
      '#title' => t('Weight'),
      '#default_value' => $item->weight,
      '#title_display' => 'invisible',
      '#tree' => true,
    ),
  );

  return $form;
}

/**
 * Theme callback for the  report elements form item.
 *
 * The theme callback will format the elements data structure into a table and
 * add  tabledrag functionality.
 */
function theme_amcr_element_form($variables) {
  $form = $variables['form'];
  $rows = array();
  foreach (element_children($form['items']) as $id) {
    $form['items'][$id]['weight']['#attributes']['class'] = array('amcr-element-weight');

    $rows[] = array(
      'data' => array(
        drupal_render($form['items'][$id]['id']),
        drupal_render($form['items'][$id]['weight']),
      ),
      'class' => array('draggable'),
    );
  }

  $header = array(t('Element ID'), t('Weight'));
  $table_id = 'amcr-elements-table';
  $output = theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => $table_id),
  ));

  $output .= drupal_render_children($form);
  drupal_add_tabledrag($table_id, 'order', 'sibling', 'amcr-element-weight');

  return $output;
}


/**
 * Theme callback for the  report metrics form item.
 *
 * The theme callback will format the metrics data structure into a table and
 * add tabledrag functionality.
 */
function theme_amcr_metric_form($variables) {
  $form = $variables['form'];
  $rows = array();
  foreach (element_children($form['items']) as $id) {
    $form['items'][$id]['weight']['#attributes']['class'] = array('amcr-metric-weight');

    $rows[] = array(
      'data' => array(
        drupal_render($form['items'][$id]['id']),
        drupal_render($form['items'][$id]['weight']),
      ),
      'class' => array('draggable'),
    );
  }

  $header = array(t('Metric ID'), t('Weight'));
  $table_id = 'amcr-metrics-table';
  $output = theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => $table_id),
  ));

  $output .= drupal_render_children($form);
  drupal_add_tabledrag($table_id, 'order', 'sibling', 'amcr-metric-weight');

  return $output;
}


/**
 * Form validation handler for aggregator_form_feed().
 *
 * @see aggregator_form_feed_submit()
 */
/*
function aggregator_form_feed_validate($form, &$form_state) {
  if ($form_state['values']['op'] == t('Save')) {
    // Ensure URL is valid.
    if (!valid_url($form_state['values']['url'], TRUE)) {
      form_set_error('url', t('The URL %url is invalid. Enter a fully-qualified URL, such as http://www.example.com/feed.xml.', array('%url' => $form_state['values']['url'])));
    }
    // Check for duplicate titles.
    if (isset($form_state['values']['fid'])) {
      $result = db_query("SELECT title, url FROM {aggregator_feed} WHERE (title = :title OR url = :url) AND fid <> :fid", array(':title' => $form_state['values']['title'], ':url' => $form_state['values']['url'], ':fid' => $form_state['values']['fid']));
    }
    else {
      $result = db_query("SELECT title, url FROM {aggregator_feed} WHERE title = :title OR url = :url", array(':title' => $form_state['values']['title'], ':url' => $form_state['values']['url']));
    }
    foreach ($result as $feed) {
      if (strcasecmp($feed->title, $form_state['values']['title']) == 0) {
        form_set_error('title', t('A feed named %feed already exists. Enter a unique title.', array('%feed' => $form_state['values']['title'])));
      }
      if (strcasecmp($feed->url, $form_state['values']['url']) == 0) {
        form_set_error('url', t('A feed with this URL %url already exists. Enter a unique URL.', array('%url' => $form_state['values']['url'])));
      }
    }
  }
}

/**
 * Form submission handler for aggregator_form_feed().
 *
 * @see aggregator_form_feed_validate()
 *
 * @todo Add delete confirmation dialog.
 */
/*
function aggregator_form_feed_submit($form, &$form_state) {
  if ($form_state['values']['op'] == t('Delete')) {
    $title = $form_state['values']['title'];
    // Unset the title.
    unset($form_state['values']['title']);
  }
  aggregator_save_feed($form_state['values']);
  if (isset($form_state['values']['fid'])) {
    if (isset($form_state['values']['title'])) {
      drupal_set_message(t('The feed %feed has been updated.', array('%feed' => $form_state['values']['title'])));
      if (arg(0) == 'admin') {
        $form_state['redirect'] = 'admin/config/services/aggregator/';
        return;
      }
      else {
        $form_state['redirect'] = 'aggregator/sources/' . $form_state['values']['fid'];
        return;
      }
    }
    else {
      watchdog('aggregator', 'Feed %feed deleted.', array('%feed' => $title));
      drupal_set_message(t('The feed %feed has been deleted.', array('%feed' => $title)));
      if (arg(0) == 'admin') {
        $form_state['redirect'] = 'admin/config/services/aggregator/';
        return;
      }
      else {
        $form_state['redirect'] = 'aggregator/sources/';
        return;
      }
    }
  }
  else {
    watchdog('aggregator', 'Feed %feed added.', array('%feed' => $form_state['values']['title']), WATCHDOG_NOTICE, l(t('view'), 'admin/config/services/aggregator'));
    drupal_set_message(t('The feed %feed has been added.', array('%feed' => $form_state['values']['title'])));
  }
}

/**
 * Deletes a feed.
 *
 * @param $feed
 *   An associative array describing the feed to be cleared.
 *
 * @see aggregator_admin_remove_feed_submit()
 */
/*
function aggregator_admin_remove_feed($form, $form_state, $feed) {
  return confirm_form(
    array(
      'feed' => array(
        '#type' => 'value',
        '#value' => $feed,
      ),
    ),
    t('Are you sure you want to remove all items from the feed %feed?', array('%feed' => $feed->title)),
    'admin/config/services/aggregator',
    t('This action cannot be undone.'),
    t('Remove items'),
    t('Cancel')
  );
}

/**
 * Form submission handler for aggregator_admin_remove_feed().
 *
 * Removes all items from a feed and redirects to the overview page.
 */
/*
function aggregator_admin_remove_feed_submit($form, &$form_state) {
  aggregator_remove($form_state['values']['feed']);
  $form_state['redirect'] = 'admin/config/services/aggregator';
}

/**
 * Form constructor for importing feeds from OPML.
 *
 * @ingroup forms
 * @see aggregator_form_opml_validate()
 * @see aggregator_form_opml_submit()
 */
/*
function aggregator_form_opml($form, &$form_state) {
  $period = drupal_map_assoc(array(900, 1800, 3600, 7200, 10800, 21600, 32400, 43200, 64800, 86400, 172800, 259200, 604800, 1209600, 2419200), 'format_interval');

  $form['upload'] = array(
    '#type' => 'file',
    '#title' => t('OPML File'),
    '#description' => t('Upload an OPML file containing a list of feeds to be imported.'),
  );
  $form['remote'] = array(
    '#type' => 'textfield',
    '#title' => t('OPML Remote URL'),
    '#maxlength' => 1024,
    '#description' => t('Enter the URL of an OPML file. This file will be downloaded and processed only once on submission of the form.'),
  );
  $form['refresh'] = array(
    '#type' => 'select',
    '#title' => t('Update interval'),
    '#default_value' => 3600,
    '#options' => $period,
    '#description' => t('The length of time between feed updates. Requires a correctly configured <a href="@cron">cron maintenance task</a>.', array('@cron' => url('admin/reports/status'))),
  );
  $form['block'] = array('#type' => 'select',
    '#title' => t('News items in block'),
    '#default_value' => 5,
    '#options' => drupal_map_assoc(array(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20)),
    '#description' => t("Drupal can make a block with the most recent news items of a feed. You can <a href=\"@block-admin\">configure blocks</a> to be displayed in the sidebar of your page. This setting lets you configure the number of news items to show in a feed's block. If you choose '0' these feeds' blocks will be disabled.", array('@block-admin' => url('admin/structure/block'))),
  );

  // Handling of categories.
  $options = array_map('check_plain', db_query("SELECT cid, title FROM {aggregator_category} ORDER BY title")->fetchAllKeyed());
  if ($options) {
    $form['category'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Categorize news items'),
      '#options' => $options,
      '#description' => t('New feed items are automatically filed in the checked categories.'),
    );
  }
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import')
  );

  return $form;
}

/**
 * Form validation handler for aggregator_form_opml().
 *
 * @see aggregator_form_opml_submit()
 */
/*
function aggregator_form_opml_validate($form, &$form_state) {
  // If both fields are empty or filled, cancel.
  if (empty($form_state['values']['remote']) == empty($_FILES['files']['name']['upload'])) {
    form_set_error('remote', t('You must <em>either</em> upload a file or enter a URL.'));
  }

  // Validate the URL, if one was entered.
  if (!empty($form_state['values']['remote']) && !valid_url($form_state['values']['remote'], TRUE)) {
    form_set_error('remote', t('This URL is not valid.'));
  }
}

/**
 * Form submission handler for aggregator_form_opml().
 *
 * @see aggregator_form_opml_validate()
 */
/*
function aggregator_form_opml_submit($form, &$form_state) {
  $data = '';
  $validators = array('file_validate_extensions' => array('opml xml'));
  if ($file = file_save_upload('upload', $validators)) {
    $data = file_get_contents($file->uri);
  }
  else {
    $response = drupal_http_request($form_state['values']['remote']);
    if (!isset($response->error)) {
      $data = $response->data;
    }
  }

  $feeds = _aggregator_parse_opml($data);
  if (empty($feeds)) {
    drupal_set_message(t('No new feed has been added.'));
    return;
  }

  $form_state['values']['op'] = t('Save');

  foreach ($feeds as $feed) {
    // Ensure URL is valid.
    if (!valid_url($feed['url'], TRUE)) {
      drupal_set_message(t('The URL %url is invalid.', array('%url' => $feed['url'])), 'warning');
      continue;
    }

    // Check for duplicate titles or URLs.
    $result = db_query("SELECT title, url FROM {aggregator_feed} WHERE title = :title OR url = :url", array(':title' => $feed['title'], ':url' => $feed['url']));
    foreach ($result as $old) {
      if (strcasecmp($old->title, $feed['title']) == 0) {
        drupal_set_message(t('A feed named %title already exists.', array('%title' => $old->title)), 'warning');
        continue 2;
      }
      if (strcasecmp($old->url, $feed['url']) == 0) {
        drupal_set_message(t('A feed with the URL %url already exists.', array('%url' => $old->url)), 'warning');
        continue 2;
      }
    }

    $form_state['values']['title'] = $feed['title'];
    $form_state['values']['url'] = $feed['url'];
    drupal_form_submit('aggregator_form_feed', $form_state);
  }

  $form_state['redirect'] = 'admin/config/services/aggregator';
}

/**
 * Parses an OPML file.
 *
 * Feeds are recognized as <outline> elements with the attributes "text" and
 * "xmlurl" set.
 *
 * @param $opml
 *   The complete contents of an OPML document.
 *
 * @return
 *   An array of feeds, each an associative array with a "title" and a "url"
 *   element, or NULL if the OPML document failed to be parsed. An empty array
 *   will be returned if the document is valid but contains no feeds, as some
 *   OPML documents do.
 */
/*
function _aggregator_parse_opml($opml) {
  $feeds = array();
  $xml_parser = drupal_xml_parser_create($opml);
  if (xml_parse_into_struct($xml_parser, $opml, $values)) {
    foreach ($values as $entry) {
      if ($entry['tag'] == 'OUTLINE' && isset($entry['attributes'])) {
        $item = $entry['attributes'];
        if (!empty($item['XMLURL']) && !empty($item['TEXT'])) {
          $feeds[] = array('title' => $item['TEXT'], 'url' => $item['XMLURL']);
        }
      }
    }
  }
  xml_parser_free($xml_parser);

  return $feeds;
}

/**
 * Page callback: Refreshes a feed, then redirects to the overview page.
 *
 * @param $feed
 *   An object describing the feed to be refreshed.
 */
/*
function aggregator_admin_refresh_feed($feed) {
  if (!isset($_GET['token']) || !drupal_valid_token($_GET['token'], 'aggregator/update/' . $feed->fid)) {
    return MENU_ACCESS_DENIED;
  }
  aggregator_refresh($feed);
  drupal_goto('admin/config/services/aggregator');
}

/**
 * Form constructor for the aggregator system settings.
 *
 * @see aggregator_admin_form_submit()
 * @ingroup forms
 */
/*
function aggregator_admin_form($form, $form_state) {
  // Global aggregator settings.
  $form['aggregator_allowed_html_tags'] = array(
    '#type' => 'textfield',
    '#title' => t('Allowed HTML tags'),
    '#size' => 80,
    '#maxlength' => 255,
    '#default_value' => variable_get('aggregator_allowed_html_tags', '<a> <b> <br> <dd> <dl> <dt> <em> <i> <li> <ol> <p> <strong> <u> <ul>'),
    '#description' => t('A space-separated list of HTML tags allowed in the content of feed items. Disallowed tags are stripped from the content.'),
  );

  // Make sure configuration is sane.
  aggregator_sanitize_configuration();

  // Get all available fetchers.
  $fetchers = module_implements('aggregator_fetch');
  foreach ($fetchers as $k => $module) {
    if ($info = module_invoke($module, 'aggregator_fetch_info')) {
      $label = $info['title'] . ' <span class="description">' . $info['description'] . '</span>';
    }
    else {
      $label = $module;
    }
    unset($fetchers[$k]);
    $fetchers[$module] = $label;
  }

  // Get all available parsers.
  $parsers = module_implements('aggregator_parse');
  foreach ($parsers as $k => $module) {
    if ($info = module_invoke($module, 'aggregator_parse_info')) {
      $label = $info['title'] . ' <span class="description">' . $info['description'] . '</span>';
    }
    else {
      $label = $module;
    }
    unset($parsers[$k]);
    $parsers[$module] = $label;
  }

  // Get all available processors.
  $processors = module_implements('aggregator_process');
  foreach ($processors as $k => $module) {
    if ($info = module_invoke($module, 'aggregator_process_info')) {
      $label = $info['title'] . ' <span class="description">' . $info['description'] . '</span>';
    }
    else {
      $label = $module;
    }
    unset($processors[$k]);
    $processors[$module] = $label;
  }

  // Only show basic configuration if there are actually options.
  $basic_conf = array();
  if (count($fetchers) > 1) {
    $basic_conf['aggregator_fetcher'] = array(
      '#type' => 'radios',
      '#title' => t('Fetcher'),
      '#description' => t('Fetchers download data from an external source. Choose a fetcher suitable for the external source you would like to download from.'),
      '#options' => $fetchers,
      '#default_value' => variable_get('aggregator_fetcher', 'aggregator'),
    );
  }
  if (count($parsers) > 1) {
    $basic_conf['aggregator_parser'] = array(
      '#type' => 'radios',
      '#title' => t('Parser'),
      '#description' => t('Parsers transform downloaded data into standard structures. Choose a parser suitable for the type of feeds you would like to aggregate.'),
      '#options' => $parsers,
      '#default_value' => variable_get('aggregator_parser', 'aggregator'),
    );
  }
  if (count($processors) > 1) {
    $basic_conf['aggregator_processors'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Processors'),
      '#description' => t('Processors act on parsed feed data, for example they store feed items. Choose the processors suitable for your task.'),
      '#options' => $processors,
      '#default_value' => variable_get('aggregator_processors', array('aggregator')),
    );
  }
  if (count($basic_conf)) {
    $form['basic_conf'] = array(
      '#type' => 'fieldset',
      '#title' => t('Basic configuration'),
      '#description' => t('For most aggregation tasks, the default settings are fine.'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      );
    $form['basic_conf'] += $basic_conf;
  }

  // Implementing modules will expect an array at $form['modules'].
  $form['modules'] = array();

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );

  return $form;
}

/**
 * Form submission handler for aggregator_admin_form().
 */
/*
function aggregator_admin_form_submit($form, &$form_state) {
  if (isset($form_state['values']['aggregator_processors'])) {
    $form_state['values']['aggregator_processors'] = array_filter($form_state['values']['aggregator_processors']);
  }
  system_settings_form_submit($form, $form_state);
}

/**
 * Form constructor to add/edit/delete aggregator categories.
 *
 * @param $edit
 *   An associative array containing:
 *   - title: A string to use for the category title.
 *   - description: A string to use for the category description.
 *   - cid: The category ID.
 *
 * @ingroup forms
 * @see aggregator_form_category_validate()
 * @see aggregator_form_category_submit()
 */
/*
function aggregator_form_category($form, &$form_state, $edit = array('title' => '', 'description' => '', 'cid' => NULL)) {
  $form['title'] = array('#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => $edit['title'],
    '#maxlength' => 64,
    '#required' => TRUE,
  );
  $form['description'] = array('#type' => 'textarea',
    '#title' => t('Description'),
    '#default_value' => $edit['description'],
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save'));
  if ($edit['cid']) {
    $form['actions']['delete'] = array('#type' => 'submit', '#value' => t('Delete'));
    $form['cid'] = array('#type' => 'hidden', '#value' => $edit['cid']);
  }

  return $form;
}

/**
 * Form validation handler for aggregator_form_category().
 *
 * @see aggregator_form_category_submit()
 */
/*
function aggregator_form_category_validate($form, &$form_state) {
  if ($form_state['values']['op'] == t('Save')) {
    // Check for duplicate titles
    if (isset($form_state['values']['cid'])) {
      $category = db_query("SELECT cid FROM {aggregator_category} WHERE title = :title AND cid <> :cid", array(':title' => $form_state['values']['title'], ':cid' => $form_state['values']['cid']))->fetchObject();
    }
    else {
      $category = db_query("SELECT cid FROM {aggregator_category} WHERE title = :title", array(':title' => $form_state['values']['title']))->fetchObject();
    }
    if ($category) {
      form_set_error('title', t('A category named %category already exists. Enter a unique title.', array('%category' => $form_state['values']['title'])));
    }
  }
}

/**
 * Form submission handler for aggregator_form_category().
 *
 * @see aggregator_form_category_validate()
 *
 * @todo Add delete confirmation dialog.
 */
/*
function aggregator_form_category_submit($form, &$form_state) {
  if ($form_state['values']['op'] == t('Delete')) {
    $title = $form_state['values']['title'];
    // Unset the title.
    unset($form_state['values']['title']);
  }
  aggregator_save_category($form_state['values']);
  if (isset($form_state['values']['cid'])) {
    if (isset($form_state['values']['title'])) {
      drupal_set_message(t('The category %category has been updated.', array('%category' => $form_state['values']['title'])));
      if (arg(0) == 'admin') {
        $form_state['redirect'] = 'admin/config/services/aggregator/';
        return;
      }
      else {
        $form_state['redirect'] = 'aggregator/categories/' . $form_state['values']['cid'];
        return;
      }
    }
    else {
      watchdog('aggregator', 'Category %category deleted.', array('%category' => $title));
      drupal_set_message(t('The category %category has been deleted.', array('%category' => $title)));
      if (arg(0) == 'admin') {
        $form_state['redirect'] = 'admin/config/services/aggregator/';
        return;
      }
      else {
        $form_state['redirect'] = 'aggregator/categories/';
        return;
      }
    }
  }
  else {
    watchdog('aggregator', 'Category %category added.', array('%category' => $form_state['values']['title']), WATCHDOG_NOTICE, l(t('view'), 'admin/config/services/aggregator'));
    drupal_set_message(t('The category %category has been added.', array('%category' => $form_state['values']['title'])));
  }
}

*/
