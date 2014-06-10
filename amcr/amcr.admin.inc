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

  $form['elements'] = _amcr_elements_list($form_state,
    !empty($report->elements) ? $report->elements : array());

  $form['metrics'] = _amcr_metrics_list($form_state,
    !empty($report->metrics) ? $report->metrics : array());


  $form['date_from'] = array('#type' => 'date',
    '#title' => t('Date from'),
    '#default_value' => isset($report->date_from) ?
        array('year' => date('Y', intval($report->date_from)),
          'month' => date('m', intval($report->date_from)),
          'day' => date('d', intval($report->date_from))
        ) : '',
    '#maxlength' => NULL,
    '#description' => t('Data is analysed since \'Date from \' till \'Date to\'. It is '.
      'used if \'Period\' is not specified '),
    '#required' => FALSE,
  );

  $form['date_to'] = array('#type' => 'date',
    '#title' => t('Date to'),
    '#default_value' => isset($report->date_to) ?
        array('year' => date('Y', intval($report->date_to)),
        'month' => date('m', intval($report->date_to)),
        'day' => date('d', intval($report->date_to))
        ) : '',
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
  $form['sort_by'] = array('#type' => 'textfield',
    '#title' => t('Order by'),
    '#default_value' => isset($report->sort_by) ? $report->sort_by : '',
    '#description' => t('Metric id to sort results'),
    '#required' => FALSE,
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
 * Generates form structure for elements list
 *
 * @param &$form_state
 *  the function change $form_state['elements_count'] parameter
 * @param $report_elements
 *  $report->elements object restored from db
 *
 * @return
 *  $form['elements'] structure
 */
function _amcr_elements_list(&$form_state = NULL, $report_elements) {

  $form = array();

  if (isset($form_state['elements_count'])) {
    $elements_count = $form_state['elements_count'];
  }
  else {
    $elements_count = max(2, empty($report_elements) ? 2 : count($report_elements));
  }

  $form['#title'] = t('Elements');
  $form['#tree'] = true;
  $form['#description'] = t('A list of elements that breaks down '.
    '(organizes) the metrics data in the report');
  $form['#theme'] = 'amcr_element_form';
  $form['#prefix'] = '<div id="amcr-elements-wrapper">';
  $form['#suffix'] = '</div>';

  $delta = 0;
  $weight = 0;
  foreach ($report_elements as $item) {
    $form['items'][$item->id] = _amcr_element_form($item);
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
    $form['items'][$key] = _amcr_element_form($item);
  }

  $form['add'] = array(
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

  return $form;
}


/*
 * Generates form structure for metrics list
 *
 * @param &$form_state
 *  the function change $form_state['metrics_count'] parameter
 * @param $report_metrics
 *  $report->metrics object restored from db
 *
 * @return
 *  $form['metrics'] structure
 */
function _amcr_metrics_list(&$form_state = NULL, $report_metrics) {

  $form = array();

  if (isset($form_state['metrics_count'])) {
    $metrics_count = $form_state['metrics_count'];
  }
  else {
    $metrics_count = max(2, empty($report_metrics) ? 2 : count($report_metrics));
  }

  $form['#title'] = t('Metrics');
  $form['#description'] = 'A list of the events to include in the report.'
    .' A report must specify at least one metric (Ranked/Overtime reports'
    .' support one or more metrics reportDefinitionMetricList metrics.'
    .' Trended reports support only one metric.)';
  $form['#tree'] = true;
  $form['#theme'] = 'amcr_metric_form';
  $form['#prefix'] = '<div id="amcr-metrics-wrapper">';
  $form['#suffix'] = '</div>';

  $delta = 0;
  $weight = 0;
  foreach ($report_metrics as $item) {
    $form['items'][$item->id] = _amcr_metric_form($item);
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
    $form['items'][$key] = _amcr_metric_form($item);
  }

  $form['add'] = array(
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

  $output = '<label>' . $form['#title'] . '</label>';
  $header = array(t('Element ID'), t('Weight'));
  $table_id = 'amcr-elements-table';
  $output .= theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => $table_id),
  ));
  $output .= '<div class="description">' . $form['#description'] . '</div>';
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
  $output = '<label>' . $form['#title'] . '</label>';
  $output .= theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => $table_id),
  ));
  $output .= '<div class="description">' . $form['#description'] . '</div>';
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
 * Form submission handler for amcr_report_form().
 *
 * @see amcr_report_form_validate()
 *
 */
function amcr_report_form_submit($form, &$form_state) {
  if ($form_state['values']['op'] == t('Delete')) {
    $title = $form_state['values']['title'];
    unset($form_state['values']['title']);
  }

  amcr_save_report_settings($form_state['values']);

  if (isset($form_state['values']['id'])) {
    if (isset($form_state['values']['title'])) {
      drupal_set_message(t('The report %report has been updated.', array('%report' => $form_state['values']['title'])));
      $form_state['redirect'] = 'admin/config/services/amcr/';
      return;
    }
    else {
      watchdog('amcr', 'Report %report deleted.', array('%report' => $title));
      drupal_set_message(t('The report %report has been deleted.', array('%report' => $title)));
      $form_state['redirect'] = 'admin/config/services/amcr/';
      return;
    }
  }
  else {
    watchdog('amcr', 'Report %report added.', array('%report' => $form_state['values']['title']), WATCHDOG_NOTICE, l(t('view'), 'admin/config/services/amcr'));
    drupal_set_message(t('The report %report has been added.', array('%report' => $form_state['values']['title'])));
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







