<?php

/**
 * @file
 * Administration page callbacks for the Aggregator module.
 */


/**
 * Displays the reports list.
 *
 * @return string
 *   A HTML-formatted string with list of reports.
 */
function amcr_report_list() {
  $result = db_query('SELECT rd.id, rd.name, rd.last_update, rd.update_frequency'
    . ' FROM {amcr_report_description} rd ORDER BY rd.name');

  $output = '<h3>' . t('Report list') . '</h3>';

  $header = array(t('ID'), t('Name'), t('Last update'), t('Next update'),
    array('data' => t('Operations'), 'colspan' => '3'));

  $rows = array();
  foreach ($result as $report) {
    $rows[] = array(
      $report->id,
      $report->name,
      ($report->last_update ? format_date($report->last_update, 'long') : t('never')),
      ($report->update_frequency ? format_date($report->last_update + $report->update_frequency, 'long') : t('never')),
      l(t('edit'), "admin/config/services/amcr/edit/$report->id"),
      l(t('remove'), "admin/config/services/amcr/remove/$report->id"),
      l(t('run now'), "admin/config/services/amcr/update/$report->id"),
    );
  }
  $output .= theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'empty' => t('No reports available. <a href="@link">Add report</a>.',
      array('@link' => url('admin/config/services/amcr/add/')))));

  return $output;
}

/**
 * Form constructor for amcr general settings.
 *
 * @ingroup forms
 */
function amcr_admin_form($form, &$form_state) {
  $form['amcr_username'] = array(
    '#type' => 'textfield',
    '#title' => t('API Username'),
    '#default_value' => variable_get('amcr_username', ''),
    '#maxlength' => NULL,
    '#required' => TRUE,
  );
  $form['amcr_secret'] = array(
    '#type' => 'textfield',
    '#title' => t('Shared secret'),
    '#default_value' => variable_get('amcr_secret', ''),
    '#maxlength' => NULL,
    '#required' => TRUE,
  );

  return system_settings_form($form);
}


/**
 * Form constructor for adding and editing report config.
 *
 * @param AmcrReportConfig $report
 *   (optional) If editing a report, the report to edit as a PHP stdClass value;
 *   if adding a new report, NULL. Defaults to NULL.
 *
 * @ingroup forms
 * @see amcr_form_report_validate()
 * @see amcr_form_report_submit()
 */
function amcr_report_form($form, &$form_state, AmcrReportConfig $report = NULL) {

  $method_options = array(
    'ranked' => 'Ranked report',
    'trended' => 'Trended report',
    'overtime' => 'Overtime report',
    //'realtime' => 'Real time report',
  );
  $date_granularity_options = array(
    'none' => '',
    'hour' => 'hour',
    'dae' => 'day',
    'week' => 'week',
    'quarter' => 'quarter',
    'year' => 'year',
  );

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => isset($report->name) ? $report->name : '',
    '#maxlength' => 255,
    '#description' => t('The name of the report. It is shown in list of reports.'),
    '#required' => TRUE,
  );
  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#default_value' => isset($report->description) ? $report->description : '',
    '#maxlength' => NULL,
    '#description' => t('It is shown or report edit page. Admins should leave their notes about the report here.'),
    '#required' => TRUE,
  );
  $form['report_suite_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Report suite id'),
    '#default_value' => isset($report->report_suite_id) ? $report->report_suite_id : '',
    '#maxlength' => NULL,
    '#description' => t('Id of omniture report suite'),
    '#required' => TRUE,
  );
  $form['method'] = array(
    '#type' => 'select',
    '#title' => t('Report type'),
    '#default_value' => isset($report->method) ? $report->method : 'ranked',
    '#options' => $method_options,
  );

  $form['elements'] = _amcr_elements_list($form_state,
    !empty($report->elements) ? $report->elements : array());

  $form['metrics'] = _amcr_metrics_list($form_state,
    !empty($report->metrics) ? $report->metrics : array());

  $form['date_granularity'] = array(
    '#type' => 'select',
    '#title' => t('Date granularity'),
    '#default_value' => isset($report->date_granularity) ? $report->date_granularity : 'none',
    '#description' => t('A time period used to display report data.'),
    '#required' => FALSE,
    '#options' => $date_granularity_options,
  );

  $form['date_from'] = array(
    '#type' => 'date',
    '#title' => t('Date from'),
    '#default_value' => isset($report->date_from) ?
    array(
      'year' => date('Y', intval($report->date_from)),
      'month' => date('n', intval($report->date_from)),
      'day' => date('j', intval($report->date_from)),
    ) : '',
    '#maxlength' => NULL,
    '#description' => t("Data is analysed since 'Date from' till 'Date to'. It is
      not used if 'Period' is not specified "),
    '#required' => FALSE,
  );

  $form['date_to'] = array(
    '#type' => 'date',
    '#title' => t('Date to'),
    '#default_value' => isset($report->date_to) ?
    array(
      'year' => date('Y', intval($report->date_to)),
      'month' => date('n', intval($report->date_to)),
      'day' => date('j', intval($report->date_to)),
    ) : '',
    '#maxlength' => NULL,
    '#description' => t("Data is analysed since 'Date from ' till 'Date to'.'
     . ' It is not used if 'Period' is not specified."),
    '#required' => FALSE,
  );
  $form['period'] = array(
    '#type' => 'textfield',
    '#title' => t('Period'),
    '#default_value' => isset($report->period) ? $report->period / 86400 : '',
    '#maxlength' => NULL,
    '#size' => 5,
    '#description' => t('Period in days for getting data. It is counted from the moment report execution.'),
    '#required' => FALSE,
    '#element_validate' => array('amcr_int_number_validate'),
    '#error_title' => t('Period'),
  );
  $form['sort_by'] = array(
    '#type' => 'textfield',
    '#title' => t('Order by'),
    '#default_value' => isset($report->sort_by) ? $report->sort_by : '',
    '#description' => t('Metric id to sort results'),
    '#required' => FALSE,
  );
  $form['update_frequency'] = array(
    '#type' => 'textfield',
    '#title' => t('Update frequency'),
    '#default_value' => isset($report->update_frequency) ? $report->update_frequency / 60 : '',
    '#maxlength' => NULL,
    '#size' => 5,
    '#description' => t('Number of minutes between running report update procedure.'),
    '#required' => FALSE,
    '#element_validate' => array('amcr_int_number_validate'),
    '#error_title' => t('Update frequency'),
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
 * Implements 'Add element' submit handler.
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
 * Implements 'Add metric' submit handler.
 */
function amcr_add_metric_submit($form, &$form_state) {
  if ($form_state['values']['metrics']['add']) {
    $n = $_GET['q'] == 'system/ajax' ? 1 : 5;
    $form_state['metrics_count'] = count($form_state['values']['metrics']['items']) + $n;
  }

  unset($form_state['input']['metrics']);
  $form_state['rebuild'] = TRUE;
}

/**
 * Generates form structure for element item.
 *
 * @param StdClass $item
 *   report element
 */
function _amcr_element_form($item = NULL) {
  $types = array(
    'OR' => 'OR',
    'AND' => 'AND',
    'NOT' => 'NOT',
  );

  $form = array(
    'id' => array(
      '#type' => 'textfield',
      '#default_value' => isset($item->element_id) ? $item->element_id : '',
      '#maxlength' => 255,
      '#size' => 30,
    ),
    'top' => array(
      '#type' => 'textfield',
      '#default_value' => isset($item->top) ? $item->top : '',
      '#maxlength' => 5,
      '#size' => 2,
      '#element_validate' => array('amcr_int_number_validate'),
      '#error_title' => t('Top'),
    ),
    'starting_with' => array(
      '#type' => 'textfield',
      '#default_value' => isset($item->starting_with) ? $item->starting_with : '',
      '#maxlength' => 5,
      '#size' => 2,
      '#element_validate' => array('amcr_int_number_validate'),
      '#error_title' => t('Starting with'),
    ),
    'keywords' => array(
      '#type' => 'textfield',
      '#default_value' => isset($item->keywords) ? $item->keywords : '',
      '#maxlength' => 255,
      '#size' => 100,
    ),
    'search_type' => array(
      '#type' => 'select',
      '#default_value' => isset($item->search_type) ? $item->search_type : 'OR',
      '#options' => $types,
    ),
    'weight' => array(
      '#type' => 'weight',
      '#default_value' => $item->weight,
      '#title_display' => 'invisible',
    ),
  );

  return $form;
}


/**
 * Generates form structure for elements list.
 *
 * @param array &$form_state
 *   the function change $form_state['elements_count'] parameter
 * @param array $report_elements
 *   $report->elements array restored from db
 *
 * @return array
 *   $form['elements'] structure
 */
function _amcr_elements_list(&$form_state = NULL, $report_elements = NULL) {

  $form = array();

  if (isset($form_state['elements_count'])) {
    $elements_count = $form_state['elements_count'];
  }
  else {
    $elements_count = max(2, empty($report_elements) ? 2 : count($report_elements));
  }

  $form['#title'] = t('Elements');
  $form['#tree'] = TRUE;
  $form['#description'] = t('A list of elements that breaks down
   (organizes) the metrics data in the report. Parameters:
    "Top" - (Optional) Specifies the number of rows in the report to return. Use with
    startingWith to generate paged reports. For example, top=5 returns five. rows.
    "Starting with" - (Optional) Specifies the first row in the report to return. Use with top to
    generate paged reports. For example, startingWith=20 returns report rows starting at row 20.
    "Keywords" - a list of keywords to include or exclude from the search, based on the type.
    Keywords must be separated by spaces, Keyword values can also leverage the following special
    characters to define advanced search criteria:
    * Wild Card (e.g. "page*.html")
    ^ Starts With (e.g. "^http://")
    $ Ends With (e.g. ".html$")
    "Search type" - boolean values used to link multiple search terms in a report search.
  ');
  $form['#theme'] = 'amcr_element_form';
  $form['#prefix'] = '<div id="amcr-elements-wrapper">';
  $form['#suffix'] = '</div>';

  $delta = 0;
  $weight = 0;

  if (isset($form_state['values']['elements']['add'])) {
    foreach ($form_state['values']['elements']['items'] as $key => $element) {
      $item = new stdClass();
      $item->element_id = $element['id'];
      $item->top = $element['top'];
      $item->starting_with = $element['starting_with'];
      $item->keywords = $element['keywords'];
      $item->search_type = $element['search_type'];
      $item->weight = $element['weight'];
      $form['items'][$key] = _amcr_element_form($item);
      $delta++;
      if ($item->weight > $weight) {
        $weight = $item->weight;
      }
    }
  }
  else {
    foreach ($report_elements as $item) {
      $form['items'][$item->id] = _amcr_element_form($item);
      $delta++;
      if ($item->weight > $weight) {
        $weight = $item->weight;
      }
    }
  }

  for ($delta; $delta < $elements_count; $delta++) {
    $key = 'new:' . $delta;
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


/**
 * Generates form structure for metrics list.
 *
 * @param array &$form_state
 *   the function change $form_state['metrics_count'] parameter
 * @param array $report_metrics
 *   $report->metrics array restored from db
 *
 * @return array
 *   $form['metrics'] structure
 */
function _amcr_metrics_list(&$form_state = NULL, $report_metrics = NULL) {
  $form = array();

  if (isset($form_state['metrics_count'])) {
    $metrics_count = $form_state['metrics_count'];
  }
  else {
    $metrics_count = max(2, empty($report_metrics) ? 2 : count($report_metrics));
  }

  $form['#title'] = t('Metrics');
  $form['#description'] = t('A list of the events to include in the report.'
    . ' A report must specify at least one metric (Ranked/Overtime reports'
    . ' support one or more metrics reportDefinitionMetricList metrics.'
    . ' Trended reports support only one metric');
  $form['#tree'] = TRUE;
  $form['#theme'] = 'amcr_metric_form';
  $form['#prefix'] = '<div id="amcr-metrics-wrapper">';
  $form['#suffix'] = '</div>';

  $delta = 0;
  $weight = 0;
  if (isset($form_state['values']['metrics']['add'])) {
    foreach ($form_state['values']['metrics']['items'] as $key => $metric) {
      $item = new stdClass();
      $item->metric_id = $metric['id'];
      $item->weight = $metric['weight'];
      $form['items'][$key] = _amcr_metric_form($item);
      $delta++;
      if ($item->weight > $weight) {
        $weight = $item->weight;
      }
    }
  }
  else {
    foreach ($report_metrics as $item) {
      $form['items'][$item->id] = _amcr_metric_form($item);
      $delta++;
      if ($item->weight > $weight) {
        $weight = $item->weight;
      }
    }
  }

  for ($delta; $delta < $metrics_count; $delta++) {
    $key = 'new:' . $delta;
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


/**
 * Generates form structure for metric item.
 *
 * @param object $item
 *   report metric
 */
function _amcr_metric_form($item = NULL) {
  $form = array(
    '#tree' => TRUE,
    'id' => array(
      '#type' => 'textfield',
      '#default_value' => isset($item->metric_id) ? $item->metric_id : '',
      '#maxlength' => 100,
      '#tree' => TRUE,
    ),
    'weight' => array(
      '#type' => 'weight',
      '#title' => t('Weight'),
      '#default_value' => $item->weight,
      '#title_display' => 'invisible',
      '#tree' => TRUE,
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
        drupal_render($form['items'][$id]['top']),
        drupal_render($form['items'][$id]['starting_with']),
        drupal_render($form['items'][$id]['keywords']),
        drupal_render($form['items'][$id]['search_type']),
        drupal_render($form['items'][$id]['weight']),
      ),
      'class' => array('draggable'),
    );
  }

  $output = '<label>' . $form['#title'] . '</label>';
  $header = array(
    t('Element ID'),
    t('Results limit'),
    t('Start from'),
    t('Keywords'),
    t('Search type'),
    t('Weight'),
  );
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
 * Form submission handler for amcr_report_form().
 *
 * @see amcr_report_form_validate()
 */
function amcr_report_form_submit($form, &$form_state) {
  if ($form_state['values']['op'] == t('Delete')) {
    $form_state['redirect'] = 'admin/config/services/amcr/remove/' . $form_state['values']['id'];
    return;
  }

  $edit = $form_state['values'];
  $report_config = new AmcrReportConfig(NULL);
  $report_config->id = !empty($edit['id']) ? $edit['id'] : NULL;
  $report_config->name = !empty($edit['name']) ? $edit['name'] : NULL;
  $report_config->description = !empty($edit['description']) ? $edit['description'] : NULL;
  $report_config->report_suite_id = !empty($edit['report_suite_id']) ? $edit['report_suite_id'] : NULL;
  $report_config->method = !empty($edit['method']) ? $edit['method'] : NULL;
  $report_config->date_granularity = (!empty($edit['date_granularity']) &&
    $edit['date_granularity'] != 'none') ? $edit['date_granularity'] : NULL;
  $report_config->date_from = mktime(0, 0, 0, $edit['date_from']['month'],
    $edit['date_from']['day'], $edit['date_from']['year']);
  $report_config->date_to = mktime(23, 59, 59, $edit['date_to']['month'],
    $edit['date_to']['day'], $edit['date_to']['year']);
  $report_config->period = !empty($edit['period']) ? $edit['period'] * 86400 : NULL;
  $report_config->sort_by = !empty($edit['sort_by']) ? $edit['sort_by'] : NULL;
  $report_config->update_frequency = !empty($edit['update_frequency']) ? $edit['update_frequency'] * 60 : NULL;

  foreach ($edit['elements']['items'] as $id => $fields) {
    if (!empty($fields['id'])) {
      $element = new stdClass();
      if (is_numeric($id)) {
        $element->id = $id;
      }
      else {
        $element->id = NULL;
      }
      $element->element_id = $fields['id'];
      $element->weight = $fields['weight'];
      $element->top = $fields['top'];
      $element->starting_with = $fields['starting_with'];
      $element->keywords = $fields['keywords'];
      $element->search_type = $fields['search_type'];
      $element->weight = $fields['weight'];
      $report_config->elements[$id] = $element;
    }
  }

  foreach ($edit['metrics']['items'] as $id => $fields) {
    if (!empty($fields['id'])) {
      $metric = new stdClass();
      if (is_numeric($id)) {
        $metric->id = $id;
      }
      else {
        $metric->id = NULL;
      }
      $metric->metric_id = $fields['id'];
      $metric->weight = $fields['weight'];
      $report_config->metrics[$id] = $metric;
    }
  }

  $report_config->save();

  $form_state['redirect'] = 'admin/config/services/amcr/';

  if (isset($form_state['values']['id'])) {
    drupal_set_message(t('The report config %report has been updated.',
      array('%report' => $form_state['values']['name'])));
  }
  else {
    drupal_set_message(t('The report config %report has been added.',
      array('%report' => $form_state['values']['name'])));
  }
}

/**
 * Deletes a report config.
 *
 * @param AmcrReportConfig $report
 *   An AmcrReportConfig object to be deleted.
 */
function amcr_admin_remove_report($form, $form_state, $report) {
  return confirm_form(
    array(
      'report' => array(
        '#type' => 'value',
        '#value' => $report,
      ),
    ),
    t('Are you sure you want to remove config and all data of report %report?', array('%report' => $report->name)),
    'admin/config/services/amcr',
    t('This action cannot be undone.'),
    t('Remove report'),
    t('Cancel')
  );
}

/**
 * Form submission handler for amcr_admin_remove_report().
 *
 * Removes report config.
 */
function amcr_admin_remove_report_submit($form, &$form_state) {
  $report_config = $form_state['values']['report'];
  $name = $report_config->name;
  $report_config->delete();
  drupal_set_message(t('The report %report has been deleted.', array('%report' => $name)));
  $form_state['redirect'] = 'admin/config/services/amcr/';
}


/**
 * Report form period validation function.
 */
function amcr_int_number_validate($element, &$form_state) {
  if (!preg_match('/^[0-9]+$/is', $element['#value']) && !empty($element['#value'])) {
    form_error($element, t('@name must be an integer number or empty', array('@name' => $element['#error_title'])));
  }
  return $element;
}


