<?php
/**
 * Created by PhpStorm.
 * User: Yaraslau_Andrushchan
 * Date: 4/10/14
 * Time: 1:04 PM
 */

class VimnPopularityException extends Exception {

}

class VimnPopularityDataSource {

  static function get_source($config) {
    switch ($config['popularity_data_source']) {
      case 'omniture':
        return new VimnPopularityOmniture($config);
        break;
      default:
        throw new VimnPopularityException(t('Block @tab. Data source type @type is incorrect',
          array('@tab' => $config['vimn_popularity_id'],
                '@type' => $config['popularity_data_source'],
          )
        ));
        break;

    }
  }

  /**
   * returns node ids from db. Used like a stub until the method is implemented
   * in child classes
   */
  function  get_data() {

    $nodeQuery = new EntityFieldQuery();
    $entities = $nodeQuery->entityCondition('entity_type', 'node')
      ->propertyCondition('status', 1)
      ->range(0, 10);
    $entities->execute();
    $node_list = array();
    foreach ($entities->ordered_results as $entity) {
      $node = node_load($entity->entity_id);
      if (!empty($node)) {
        $node_list[] = $node;
      }
    }

    $data = array();
    $counter = 1;
    foreach($node_list as $node) {
      $data[] = array(
        'node_id' =>  $node->nid,
        'weight'  =>  $counter,
      );
      $counter++;
    }

    return $data;
  }

}

class VimnPopularityOmniture extends VimnPopularityDataSource {

  protected $config;
  protected $marketing_client;
  protected $report_suite_id;

  function __construct($config) {
    $this->config = $config;
    $this->marketing_client = NULL;
    $this->report_suite_id = variable_get('omniture_report_suite_id');
    try {
      require_once drupal_get_path(
            'module',
            'vimn_popularity'
        ) . '/adobe_marketing_sdk/lib/AdobeDigitalMarketing/Autoloader.php';
      AdobeDigitalMarketing_Autoloader::register();
      $this->marketing_client = new AdobeDigitalMarketing_Client();
      $this->marketing_client->authenticate(variable_get('omniture_api_username'), variable_get('omniture_api_secret'));
    } catch (Exception $e) {
      watchdog_exception('vimn_popularity', $e);
    }
  }

  /**
   * Retrieve all available props from omniture.
   */
  function get_props() {
    $props = array();
    if ($cached_props = cache_get($this->report_suite_id . '_props')) {
      $props = $cached_props->data;
    }
    else {
      if ($adm = $this->marketing_client) {
        $props = $adm->getReportSuiteApi()->getElements(array($this->report_suite_id), TRUE);
        cache_set($this->report_suite_id . '_props', $props, 'cache', 60 * 60);
      }
    }

    return $props;
  }


  /**
   * Extract id-views pairs from omniture response.
   */
  protected function _get_ids($data=array(), $depth=1) {

    if($depth > 0)
      return $this->_get_ids( $depth == 1 ? $data['breakdown'] : array_shift($data['breakdown']), $depth-1);

    if($depth == 0){
      $result = array();
      foreach($data as $page) {
        $result[] = array(
          'node_id' => intval($page['name']),
          'weight' => array_shift($page['counts'])
        );
      }

      return $result;
    }
  }

  /**
   * Getting the most pupular node ids from Omniture
   */
  function get_data() {

    $period = 24*3600;
    switch($this->config['time_frame']) {
      case 'last_1h':
        $period = 3600;
       break;
      case'last_12h':
        $period = 12*3600;
        break;
      case 'last_24h':
        $period = 24*3600;
        break;
      case 'last_3d':
        $period = 3*24*3600;
        break;
      case 'last_1w':
        $period = 7*24*3600;
        break;
      case 'last_1m':
        $period = 30*24*3600;
        break;
    }

    $elements = array();
    foreach($this->config['filter_fields'] as $field) {
        $elements[] = array(
         'id' => $field['filter_key'],
         'selected' => array($field['filter_value'])
        );
    }

    $depth = count($elements);

    $elements[] =  array(
      'id' => 'prop1', //must be exchaged with prop70 (node id) when viamtvuk is configured properly
      'top' => !empty($this->config['number_items']) ? $this->config['number_items'] : 10
    );

    $report_api = $this->marketing_client->getReportApi();
    $response = $report_api->queueRanked(array(
      'reportSuiteID' => $this->report_suite_id,
      'dateFrom' =>  date('Y-m-d H:i:s', time() - $period),
      'dateTo' => date('Y-m-d H:i:s'),
      'metrics'  => array(
        array('id' => 'pageviews'),
      ),
      'elements'  => $elements,
      "sortBy" => "uniquevisitors"
    ));

    $report_id = $response['reportID'];

    do {
      $report = $report_api->getReport($report_id);
      sleep(2);
    } while ($report['status'] == 'not ready');

    return $this->_get_ids($depth == 0 ? $report['report']['data'] : array_shift($report['report']['data']), $depth);
  }

}
