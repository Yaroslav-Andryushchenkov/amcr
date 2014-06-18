<?php
$plugin = array(
  'title' => t('Popularity block'),
  'description' => t('Customizable list of items based on popularity'),
  'render callback' => 'vimn_popularity_content_type_render',
  'edit form' => 'vimn_popularity_edit_form',
  'category' => t('VIMN'),
  'single' => TRUE, // Флаг, указывающий является ли плагин подтипом другого.
);

/**
 * 'Edit form' callback for the content type.
 */
function vimn_popularity_edit_form($form, &$form_state) {
  form_load_include($form_state, 'inc', 'vimn_popularity', '/plugins/content_types/vimn_popularity');
  if (empty($form_state['values']['vimn_popularity_tabs']) && !empty($form_state['conf']['vimn_popularity_tabs'])) {
    $form_state['values']['vimn_popularity_tabs'] = $form_state['conf']['vimn_popularity_tabs'];
    $form_state['tabs_count'] = count($form_state['conf']['vimn_popularity_tabs']);
  }

  $form['vimn_popularity_tabs'] = array(
    '#tree' => TRUE,
    '#prefix' => '<div id="vimn_popularity_tabs_wrapper">',
    '#suffix' => '</div>',
  );

  if (empty($form_state['tabs_count'])) {
    $form_state['tabs_count'] = 0;
  }

  // Generates form elements for tab
  $generate_tab = function ($i) use (&$form, &$form_state) {
    $form['vimn_popularity_tabs'][$i] = array(
      '#type' => 'fieldset',
      '#title' => (!empty($form_state['values']['vimn_popularity_tabs'][$i]['tab_title'])) ? $form_state['values']['vimn_popularity_tabs'][$i]['tab_title'] : t(
          'Tab ' . $i
        ),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $form['vimn_popularity_tabs'][$i]['tab_title'] = array(
      '#type' => 'textfield',
      '#title' => t('Tab title'),
      '#default_value' => (!empty($form_state['values']['vimn_popularity_tabs'][$i]['tab_title'])) ? $form_state['values']['vimn_popularity_tabs'][$i]['tab_title'] : '',
    );

    $form['vimn_popularity_tabs'][$i]['popularity_data_source'] = array(
      '#type' => 'select',
      '#title' => t('Popularity data source'),
      '#options' => array(
        'omniture' => t('Omniture'),
        'sm4' => t('SM4'),
      ),
      '#default_value' => (!empty($form_state['values']['vimn_popularity_tabs'][$i]['popularity_data_source'])) ? $form_state['values']['vimn_popularity_tabs'][$i]['popularity_data_source'] : 'omniture',
    );

    // Filter fields.
    $generate_filter_field = function ($i, $j) use (&$form, &$form_state) {
      $form['vimn_popularity_tabs'][$i]['filter_fields'][$j] = array(
        '#prefix' => '<div class="container-inline">',
        '#suffix' => '</div>',
      );


      $data_source = new VimnPopularityOmniture(array());
      $form['vimn_popularity_tabs'][$i]['filter_fields'][$j]['filter_key'] = array(
        '#type' => 'select',
        '#options' => $data_source->get_props(),
        '#default_value' => (!empty($form_state['values']['vimn_popularity_tabs'][$i]['filter_fields'][$j]['filter_key'])) ? $form_state['values']['vimn_popularity_tabs'][$i]['filter_fields'][$j]['filter_key'] : '',
      );

      $form['vimn_popularity_tabs'][$i]['filter_fields'][$j]['filter_value'] = array(
        '#type' => 'textfield',
        '#default_value' => (!empty($form_state['values']['vimn_popularity_tabs'][$i]['filter_fields'][$j]['filter_value'])) ? $form_state['values']['vimn_popularity_tabs'][$i]['filter_fields'][$j]['filter_value'] : '',
      );

      $form['vimn_popularity_tabs'][$i]['filter_fields'][$j]['remove_filter'] = array(
        '#type' => 'submit',
        '#name' => "remove_filter_{$i}_{$j}",
        '#value' => t('Remove'),
        '#submit' => array('vimn_popularity_remove_filter_field_callback'),
        '#ajax' => array(
          'wrapper' => 'filter_fields_wrapper_' . $i,
          'callback' => 'vimn_popularity_remove_filter_field_form_update_callback',
        ),
      );

    };
    $form['vimn_popularity_tabs'][$i]['filter_fields'] = array(
      '#type' => 'fieldset',
      '#title' => t('Filter fields'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#prefix' => '<div id="filter_fields_wrapper_' . $i . '">',
      '#suffix' => '</div>',
    );

    if (!isset($form_state['filter_fields_count' . $i]) && !empty($form_state['conf']['vimn_popularity_tabs'][$i]['filter_fields'])) {
      $form_state['filter_fields_count' . $i] = count($form_state['conf']['vimn_popularity_tabs'][$i]['filter_fields']);
    }
    $num_generated_filter_fields = 0;
    if (
      isset($form_state['values']['vimn_popularity_tabs'][$i]['filter_fields'])
      && is_array($form_state['values']['vimn_popularity_tabs'][$i]['filter_fields'])
    ) {
      foreach ($form_state['values']['vimn_popularity_tabs'][$i]['filter_fields'] as $key => $values) {
        $generate_filter_field($i, $key);
        $num_generated_filter_fields++;
      }
    }

    //Add new filter field
    if ($num_generated_filter_fields < $form_state['filter_fields_count' . $i]) {
      $new_key = max(array_keys($form_state['values']['vimn_popularity_tabs'][$i]['filter_fields']));
      $new_key = (is_null($new_key)) ? 1 : $new_key + 1;
      $generate_filter_field($i, $new_key);
    }

    $form['vimn_popularity_tabs'][$i]['add_filter_field'] = array(
      '#type' => 'submit',
      '#name' => 'add_filter_field_' . $i,
      '#value' => t('Add filter field'),
      '#submit' => array('vimn_popularity_add_filter_field_callback'),
      '#ajax' => array(
        'wrapper' => 'filter_fields_wrapper_' . $i,
        'callback' => 'vimn_popularity_add_filter_field_form_update_callback',
      ),
    );
    // End filter fields.

    $form['vimn_popularity_tabs'][$i]['time_frame'] = array(
      '#type' => 'select',
      '#title' => t('Time frame'),
      '#options' => array(
        'last_1h' => t('Last hour'),
        'last_12h' => t('Last 12 hours'),
        'last_24h' => t('Last 24 hours'),
        'last_3d' => t('Last 3 days'),
        'last_1w' => t('Last week'),
        'last_1m' => t('Last month'),
      ),
      '#default_value' => (!empty($form_state['values']['vimn_popularity_tabs'][$i]['time_frame'])) ? $form_state['values']['vimn_popularity_tabs'][$i]['time_frame'] : 'last_1h',
    );

    $form['vimn_popularity_tabs'][$i]['number_items'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of items(links) to display'),
      '#default_value' => (!empty($form_state['values']['vimn_popularity_tabs'][$i]['number_items'])) ? $form_state['values']['vimn_popularity_tabs'][$i]['number_items'] : '',
    );

    $form['vimn_popularity_tabs'][$i]['popularity'] = array(
      '#type' => 'select',
      '#title' => t('Popularity'),
      '#options' => array(
        'latest_items' => t('Latest items'),
        'most_popular' => t('Most popular'),
      ),
      '#default_value' => (!empty($form_state['values']['vimn_popularity_tabs'][$i]['popularity'])) ? $form_state['values']['vimn_popularity_tabs'][$i]['popularity'] : 'most_popular',
    );
    $form['vimn_popularity_tabs'][$i]['remove_tab'] = array(
      '#type' => 'submit',
      '#name' => 'remove_tab_btn_' . $i,
      '#value' => t('Remove tab'),
      '#submit' => array('vimn_popularity_remove_tab_callback'),
      '#ajax' => array(
        'wrapper' => 'vimn_popularity_tabs_wrapper',
        'callback' => 'vimn_popularity_tab_form_update_callback',
      ),
    );

    $form['vimn_popularity_tabs'][$i]['vimn_popularity_id'] = array(
      '#type' => 'hidden',
      '#value' => (!empty($form_state['values']['vimn_popularity_tabs'][$i]['vimn_popularity_id'])) ? $form_state['values']['vimn_popularity_tabs'][$i]['vimn_popularity_id'] : uniqid('vimnpopularity'),
    );

  };

  $n = 0; //Number of tabs
  // Generate tabs with elements
  if (isset($form_state['values']['vimn_popularity_tabs']) && is_array($form_state['values']['vimn_popularity_tabs'])) {
    foreach ($form_state['values']['vimn_popularity_tabs'] as $key => $values) {
      $generate_tab($key);
      $n++;
    }
  }
  //Add new tab
  if ($n < $form_state['tabs_count']) {
    $new_key = max(array_keys($form_state['values']['vimn_popularity_tabs']));
    $new_key = (is_null($new_key)) ? 1 : $new_key + 1;
    $generate_tab($new_key);
  }

  $form['add_tab'] = array(
    '#type' => 'submit',
    '#value' => t('Add tab'),
    '#submit' => array('vimn_popularity_add_tab_callback'),
    '#ajax' => array(
      'wrapper' => 'vimn_popularity_tabs_wrapper',
      'callback' => 'vimn_popularity_tab_form_update_callback',
    ),
  );

  return $form;
}

/**
 * "Add tab" button submit callback.
 */
function vimn_popularity_add_tab_callback($form, &$form_state) {
  $form_state['tabs_count']++;
  $form_state['rebuild'] = TRUE;
}

/**
 * "Remove tab" button submit callback.
 */
function vimn_popularity_remove_tab_callback($form, &$form_state) {
  if (!empty($form_state['clicked_button']['#name'])) {
    $tab_num = str_replace('remove_tab_btn_', '', $form_state['clicked_button']['#name']);
    if (isset($form_state['values']['vimn_popularity_tabs'][$tab_num])) {
      unset($form_state['values']['vimn_popularity_tabs'][$tab_num]);
      $form_state['tabs_count']--;
      $form_state['rebuild'] = TRUE;
    }
  }
}

/**
 * "Add tab" button ajax callback.
 */
function vimn_popularity_tab_form_update_callback($form, $form_state) {
  return $form['vimn_popularity_tabs'];
}

/**
 * "Add filter field" button submit callback.
 */
function vimn_popularity_add_filter_field_callback($form, &$form_state) {
  if (!empty($form_state['clicked_button']['#name'])) {
    $tab_num = str_replace('add_filter_field_', '', $form_state['clicked_button']['#name']);
    if (isset($form_state['filter_fields_count' . $tab_num])) {
      $form_state['filter_fields_count' . $tab_num]++;
    }
    else {
      $form_state['filter_fields_count' . $tab_num] = 1;
    }
    $form_state['rebuild'] = TRUE;
  }
}

/**
 * "Add filter field" button ajax callback.
 */
function vimn_popularity_add_filter_field_form_update_callback($form, $form_state) {
  if (!empty($form_state['clicked_button']['#name'])) {
    $tab_num = str_replace('add_filter_field_', '', $form_state['clicked_button']['#name']);
    if (isset($form['vimn_popularity_tabs'][$tab_num]['filter_fields'])) {
      return $form['vimn_popularity_tabs'][$tab_num]['filter_fields'];
    }
  }
}

/**
 * "Remove tab" button submit callback.
 */
function vimn_popularity_remove_filter_field_callback($form, &$form_state) {
  if (!empty($form_state['clicked_button']['#name'])) {
    list($tab_num, $filter_field_num) = explode(
      '_',
      str_replace('remove_filter_', '', $form_state['clicked_button']['#name'])
    );
    if (isset($form_state['filter_fields_count' . $tab_num])) {
      $form_state['filter_fields_count' . $tab_num]--;
      unset($form_state['values']['vimn_popularity_tabs'][$tab_num]['filter_fields'][$filter_field_num]);
      $form_state['rebuild'] = TRUE;
    }
  }
}

/**
 * "Add filter field" button ajax callback.
 */
function vimn_popularity_remove_filter_field_form_update_callback($form, $form_state) {
  if (!empty($form_state['clicked_button']['#name'])) {
    list($tab_num, $filter_field_num) = explode(
      '_',
      str_replace('remove_filter_', '', $form_state['clicked_button']['#name'])
    );
    return $form['vimn_popularity_tabs'][$tab_num]['filter_fields'];
  }
}

/**
 * Submit callback for the example_text_edit_form().
 */
function vimn_popularity_edit_form_submit($form, &$form_state) {
  $form_state['conf'] = array(
    'vimn_popularity_tabs' => $form_state['values']['vimn_popularity_tabs'],
  );
}

/**
 * Returns list of nodes by tab config.
 */
function vimn_popularity_get_tab_nodes($tab_conf) {

  $query = db_select('vimn_popularity_data', 'pd');
  $query->fields('pd', array('node_id', 'weight'));
  $query->condition('pd.tab_id', $tab_conf['vimn_popularity_id']);
  $query->orderBy('pd.weight', 'ASC');
  $query->addTag('vimn_popularity_statistic');
  $result = $query->execute();
  $node_list = array();
  foreach($result as $row) {
    $node = node_load($row->node_id);
    if(!empty($node)) {
      $node_list[] = $node;
    }
  }

  return $node_list;
}

/**
 * Render callback for the content type.
 */
function vimn_popularity_content_type_render($subtype, $conf, $args, $context) {

  $tabs = array();
  foreach ($conf['vimn_popularity_tabs'] as $tab_num => $tab_conf) {
    $tabs[$tab_num]['tab_title'] = $tab_conf['tab_title'];
    $tabs[$tab_num]['nodes'] = vimn_popularity_get_tab_nodes($tab_conf);
  }

  $block = new stdClass();
  $block->content = theme('vimn_popularity_block', array('tab_list' => $tabs));
  return $block;
}







