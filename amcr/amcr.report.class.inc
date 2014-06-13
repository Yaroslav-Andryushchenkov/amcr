<?php
/**
 * Created by PhpStorm.
 * User: Yaraslau_Andrushchan
 * Date: 5/27/14
 * Time: 8:33 PM
 */


/**
 * Implements Singletom pattern for marketion client
 */
class AmcrMarketingClient {
  static protected $marketing_client = NULL;

  static public function getInstance() {
    if(empty(self::$marketing_client))
      try {
        require_once drupal_get_path(
            'module',
            'amcr'
          ) . '/adobe_marketing_sdk/lib/AdobeDigitalMarketing/Autoloader.php';
        AdobeDigitalMarketing_Autoloader::register();
        self::$marketing_client = new AdobeDigitalMarketing_Client();
        self::$marketing_client->authenticate('dzemidziukp:MTV', '64ea1874aec2e447c11aee9db6b0bc07');
      } catch (Exception $e) {
        watchdog_exception('amcr', $e);
      }

    return self::$marketing_client;
  }
}


/**
 * Implements Ecxetption for Amcr report classes
 */
class AmcrException extends Exception {

}

/**
 * Implements api to work with report config
 */
class AmcrReportConfig {

  /**
   * Name that is shown in admin panel report list
   * can not be null
   * @var string
   */
  public $name;

  /**
   * Detailed description for report settings page
   *
   * @var string
   */
  public $description;

  /**
   * Omniture report suite id. Can not be null
   *
   * @var string
   */
  public $report_suite_id;

  /**
   * Report method (ranked, trended, overtime, realtime)
   * @var string
   */
  public $method;

  /**
   * Start date of data analysis period
   *
   * @var imestamp
   */
  public $date_from;

  /**
   * Finish date of data analysis period
   *
   * @var timestamp
   */
  public $date_to;

  /**
   * Period in seconds. Period start from 'current time' - 'period' and finises at
   * 'current time'. 'Current time' - the moment of a report execution. If it is
   * specified date_from and date_to are ignored
   *
   * @var int
   */
  public $period;

  /**
   * Number of seconds between report updates.
   * Every time cron runs it checks if current time less than last_update + update_frequency
   * if current timestamp is less, cron runs update data procedure for the report
   *
   * @var int
   */
  public $update_frequency;

  /**
   * Array of elements objects
   *
   * @var array of stdClass objects
   */
  public $elements;

  /**
   * Array of metrics objects
   *
   * @var array of stdClass objects
   */
  public $metrics;

  /**
   * @param int $report_id
   *  primary key of amcr_report_description table
   */
  public function __construct($report_id = NULL) {
    $this->metrics = array();
    $this->elements = array();
    $this->name = NULL;
    $this->description = NULL;
    $this->method = 'ranked';
    $this->date_from = NULL;
    $this->date_to = NULL;
    $this->period = NULL;
    $this->sort_by = NULL;

    if(!empty($report_id))
      $this->load($report_id);
  }

  /**
   * Load report config from database
   * @param int $report_id
   *  primary key of amcr_report_description table
   */
  public function load($report_id) {
    $report = db_query('SELECT * FROM {amcr_report_description} WHERE id = :id',
      array(':id' => $report_id))->fetchObject();

    foreach($report as $key => $value) {
      $this->$key = $value;
    }

    $result = db_query('SELECT * FROM {amcr_report_definition_element} WhERE rd_id = :rd_id ORDER BY weight ASC',
      array(':rd_id' => $report_id));
    foreach ($result as $item) {
      $this->elements[$item->id] = $item;
    }

    $result = db_query('SELECT * FROM {amcr_report_definition_metric} WhERE rd_id = :rd_id ORDER BY weight ASC',
      array(':rd_id' => $report_id));
    foreach ($result as $item) {
      $this->metrics[$item->id] = $item;
    }
  }

  /**
   * Store report config into database
   */
  public function save() {
   $status = db_merge('amcr_report_description')
      ->key(array('id' => $this->id))
      ->fields(array(
        'name' => !empty($this->name) ?  $this->name : NULL,
        'description' => !empty($this->description) ?  $this->description : NULL,
        'report_suite_id' => !empty($this->report_suite_id) ?  $this->report_suite_id : NULL,
        'method' => !empty($this->metrhod) ?  $this->method : 'ranked',
        'date_from' => !empty($this->date_from) ?  $this->date_from : NULL,
        'date_to' => !empty($this->date_to) ?  $this->date_to : NULL,
        'period' => !empty($this->period) ?  $this->period : NULL,
        'sort_by' => !empty($this->sort_by) ?  $this->sort_by : NULL,
        'update_frequency' => !empty($this->update_frequency) ?  $this->update_frequency : NULL,
      ))
      ->execute();

    if( $status == MergeQuery::STATUS_INSERT ) {
      $this->id = db_query("SELECT MAX(id) FROM {amcr_report_description}")->fetchField();
    }

    if(!empty($this->id)) {
      $result = db_query('SELECT * FROM {amcr_report_definition_element} WhERE rd_id = :rd_id ORDER BY weight ASC',
        array(':rd_id' => $this->id));
      foreach ($result as $item) {
        if(empty($this->elements[$item->id])) {
          db_delete('amcr_report_definition_element')->
            condition('id', $item->id)
            ->execute();
        }
      }
      foreach($this->elements as $element) {
        db_merge('amcr_report_definition_element')
          ->key(array('id' => $element->id))
          ->fields(array(
            'element_id' => !empty($element->element_id) ?  $element->element_id : NULL,
            'weight' => !empty($element->weight) ?  $element->weight : NULL,
            'rd_id' => $this->id,
          ))
          ->execute();
      }

      $result = db_query('SELECT * FROM {amcr_report_definition_metric} WhERE rd_id = :rd_id ORDER BY weight ASC',
        array(':rd_id' => $this->id));
      foreach ($result as $item) {
        if(empty($this->metrics[$item->id])) {
          db_delete('amcr_report_definition_metric')->
            condition('id', $item->id)
            ->execute();
        }
      }
      foreach($this->metrics as $metric) {
        db_merge('amcr_report_definition_metric')
          ->key(array('id' => $metric->id))
          ->fields(array(
            'metric_id' => !empty($metric->metric_id) ?  $metric->metric_id : NULL,
            'weight' => !empty($metric->weight) ?  $metric->weight : NULL,
            'rd_id' => $this->id,
          ))
          ->execute();
      }
    }

    if (isset($this->id)){
      watchdog('amcr', 'Report %report updated. Report Id is: %id',
        array('%report' => $this->name,'%id' => $this->id ));
    }
    else {
      watchdog('amcr', 'Report %report added. Report id is %id',
        array('%report' => $this->name,
        '%id' => $this->id),
        WATCHDOG_NOTICE, l(t('view'), 'admin/config/services/amcr'));
    }
  }


  /*
   * Deletes report config and all relative data
   */
  public function delete() {
    if(!empty($this->id)) {
      if(empty($this->name))
        $this->load($this->id);

      $name = $this->name;

      foreach($this->elements as $element) {
        db_delete('amcr_report_definition_element')->
          condition('id', $element->id)
          ->execute();
      }

      foreach($this->metrics as $metric) {
        db_delete('amcr_report_definition_metric')->
          condition('id', $metric->id)
          ->execute();
      }

      db_delete('amcr_report_description')->
        condition('id', $this->id)
        ->execute();
      watchdog('amcr', 'Report %report deleted.', array('%report' => $name));
    }
  }
}


/**
 * Gets report data from adobe marketing cloud
 */
class AmcrReport {

  /**
   * Report config
   *
   * @var AmcrReportConfig object
   */
  protected $config;

  /**
   * map of elements indexes and amcr_report_element primary keys
   *
   * @var array
   */
  protected $report_metric_ids;
  /**
   * map of metrics indexes and amcr_report_metric primary keys
   *
   * @var array
   */
  protected $report_element_ids;
  /**
   * report data returned by Adobe marketing cloud
   *
   * @var array
   */
  public $data;

  /**
   * Constructor
   *
   * @param AmcrReportConfig object
   */
  public function __construct(AmcrReportConfig $config = NULL) {
    if(empty($config)) {
      $e = new AmcrException(t('AmcrReport constructor: no config info is passed'),1);
      throw $e;
    }

    $this->config = $config;
    $this->marketing_client = AmcrMarketingClient::getInstance();
  }


  /**
   * Get Reports data from
   */
  public function getData() {

    function cmp($e1, $e2) {
      if($e1->weight == $e2->weight) {
        return 0;
      }

      return ($e1->weight < $e2->weight) ? -1 : 1;
    }

    unset($this->data);
    unset($this->report_metric_ids);
    unset($this->report_element_ids);

    $report_api = $this->marketing_client->getReportApi();

    $config_elements = $this->config->elements;
    uasort($config_elements, 'cmp');
    foreach($config_elements as $element) {
      $elements[] =  array(
        'id' => $element->element_id,
        'top' => 10,
      );
    }

    $period = 7*24*3600;

    $response = $report_api->queueRanked(array(
      'reportSuiteID' => $this->config->report_suite_id,
      'dateFrom' =>  date('Y-m-d H:i:s', time() - $period),
      'dateTo' => date('Y-m-d H:i:s'),
      'metrics'  => array(
        array('id' => 'pageviews'),
        array('id' => 'event8'),
        array('id' => 'event84'),
        array('id' => 'event86'),
      ),
      'elements'  => $elements,
      "sortBy" => "uniquevisitors"
    ));

    $report_id = $response['reportID'];

    do {
      $report = $report_api->getReport($report_id);
      sleep(2);
    } while ($report['status'] == 'not ready');

    $this->data = $report;
    $this->saveResponse($report);

    return 'data is saved';

    //return $this->_get_ids($depth == 0 ? $report['report']['data'] : array_shift($report['report']['data']), $depth);
  }


  /**
   * Saves response data in amcr_report_response table
   *
   * @param array $response
   *  A structure containing the report data of requested report. It is returned
   *  by  AdobeDigitalMarketing_Api_Report::getReport
   *
   */
  protected function saveResponse($response) {
    $report_id = $this->saveReport($response['report']);
    $insert_data = array(
      'status' => isset($response['status']) ? $response['status'] : null,
      'status_msg' => isset($response['statusMsg']) ? $response['statusMsg'] : null,
      'run_seconds' => isset($response['runSeconds']) ? $response['runSeconds'] : null,
      'wait_seconds' => isset($response['waitSeconds']) ? $response['waitSeconds'] : null,
      'status_desc' => isset($response['statusDesc']) ? $response['statusDesc'] : null,
      'report_id' => $report_id,
    );

    $insert = db_insert('amcr_report_response');
    $insert->fields(array('status', 'status_msg', 'run_seconds', 'wait_seconds', 'status_desc', 'report_id'));
    $insert->values($insert_data );
    $insert->execute();
  }

  /**
   * Saves report data in amcr_report table
   *
   * @param array $report
   *  A structure containing the report data than is nested in report response.
   *
   * @return int
   *  foreign key to amcr_report table (Id of stored report)
   *
   */
  protected function saveReport($report) {
    $metrics_id = $this->saveMetricList($report['metrics']);
    $elements_id  = $this->saveElementList($report['elements']);
    $totals_id = $this->saveCountList($report['totals']);
    $data_id = $this->saveReportDataList($report['data']);
    $insert_data = array(
      'report_suite' => isset($report['reportSuite']['id']) ? $report['reportSuite']['id'] : null,
      'period' => isset($report['period']) ? $report['period'] : null,
      'elements_id' => $elements_id,
      'metrics_id' => $metrics_id,
      'type' => isset($report['type']) ? $report['type'] : null,
      'data_id' => $data_id,
      'totals_id' => $totals_id,
    );

    $insert = db_insert('amcr_report');
    $insert->fields(array('report_suite', 'period', 'elements_id', 'metrics_id',
      'type', 'data_id', 'totals_id'));
    $insert->values($insert_data );
    $report_id = $insert->execute();

    db_update('amcr_report_element_list')
      ->condition('id', $elements_id)
      ->fields(array('report_id' => $report_id))
      ->execute();
    db_update('amcr_report_metric_list')
      ->condition('id', $metrics_id)
      ->fields(array('report_id' => $report_id))
      ->execute();
    db_update('amcr_report_count_list')
      ->condition('id', $totals_id)
      ->fields(array('report_id' => $report_id))
      ->execute();

    return $report_id;
  }


  /**
   * Saves elements list
   *
   * @param array $elemets
   *  An array of report elements.
   *
   * @return int
   *  foreign key to amcr_report_element_list table
   *
   */
  protected function saveElementList($elements) {
    $insert = db_insert('amcr_report_element_list');
    $insert->fields(array('report_id'));
    $insert->values(array('report_id' => 0));
    $list_id = $insert->execute();

    $insert = db_insert('amcr_report_element');
    $insert->fields(array('element_id', 'name', 'type', 'latency', 'current', 'list_id'));
    foreach($elements as $element) {
      $insert_data = array(
        'element_id' => isset($element['id']) ? $element['id'] : null,
        'name' => isset($element['name']) ? $element['name'] : null,
        'type' => isset($element['type']) ? $element['type'] : null,
        'latency' => isset($element['latency']) ? $element['latency'] : null,
        'current' => isset($element['current']) ? (int) $element['current'] : null,
        'list_id' => $list_id,
      );
      $insert->values($insert_data);
      $this->report_element_ids[] = $insert->execute();
    }

    return $list_id;
  }

  /**
   * Saves metrics list
   *
   * @param array $metrics
   *  An array of report metrics.
   *
   * @return int
   *  foreign key to amcr_report_metric_list table (
   *
   */
  protected function saveMetricList($metrics) {
    $insert = db_insert('amcr_report_metric_list');
    $insert->fields(array('report_id'));
    $insert->values(array('report_id' => 0));
    $list_id = $insert->execute();

    $insert = db_insert('amcr_report_metric');
    $insert->fields(array('metric_id', 'name', 'type', 'latency', 'current', 'list_id'));
    foreach($metrics as $metric) {
      $insert_data = array(
        'metric_id' => isset($metric['id']) ? $metric['id'] : null,
        'name' => isset($metric['name']) ? $metric['name'] : null,
        'type' => isset($metric['type']) ? $metric['type'] : null,
        'latency' => isset($metric['latency']) ? $metric['latency'] : null,
        'current' => isset($metric['current']) ? (int) $metric['current'] : null,
        'list_id' => $list_id,
      );
      $insert->values($insert_data);
      $this->report_metric_ids[] = $insert->execute();
    }


    return $list_id;
  }


  /**
   * Saves counts list
   *
   * @param array $counts
   *  An array of counts.
   *
   * @return int
   *  foreign key to amcr_report_count_list table
   *
   */
  protected function saveCountList($counts) {
    $insert = db_insert('amcr_report_count_list');
    $insert->fields(array('report_id'));
    $insert->values(array('report_id' => 0));
    $list_id = $insert->execute();

    $insert = db_insert('amcr_report_count');
    $insert->fields(array('list_id', 'value', 'metric_id'));
    foreach($counts as $index => $value) {
      $insert_data = array(
        'value' => $value,
        'metric_id' => isset($this->report_metric_ids[$index]) ? $this->report_metric_ids[$index] : null,
        'list_id' => $list_id,
      );
      $insert->values($insert_data);
    }

    $insert->execute();

    return $list_id;
  }

  /**
   * Saves report data array
   *
   * @param array $data
   *  An array of report data.
   *
   * @return int
   *  foreign key to amcr_report_data_list table
   *
   */
  protected function saveReportDataList($data, $depth=0) {
    $insert = db_insert('amcr_report_data_list');
    $insert->fields(array('report_id'));
    $insert->values(array('report_id' => 0));
    $list_id = $insert->execute();

    $insert = db_insert('amcr_report_data');
    $insert->fields(array('url', 'year', 'month', 'day', 'hour', 'counts_id',
      'breakdown_id', 'name', 'list_id', 'element_id'));

    foreach($data as $index => $item) {

      $counts_id = !empty($item['counts']) ? $this->saveCountList($item['counts']) : null;
      $breakdown_id = !empty($item['breakdown']) ? $this->saveReportDataList($item['breakdown'], $depth+1) : null;
      $insert_data = array(
        'name' => isset($item['name']) ? $item['name'] : null,
        'url' => isset($item['url']) ? $item['url'] : null,
        'year' => isset($item['year']) ? $item['year'] : null,
        'month' => isset($item['month']) ? $item['month'] : null,
        'day' => isset($item['day']) ? $item['day'] : null,
        'hour' => isset($item['hour']) ? $item['hour'] : null,
        'element_id' => isset($this->report_element_ids[$depth]) ? $this->report_element_ids[$depth] : null,
        'list_id' => !empty($list_id) ? $list_id : null,
        'counts_id' => !empty($counts_id) ? $counts_id : null,
        'breakdown_id' => !empty($breakdown_id) ? $breakdown_id : null,
      );
      $insert->values($insert_data);
    }

    $insert->execute();

    return $list_id;
  }
}
