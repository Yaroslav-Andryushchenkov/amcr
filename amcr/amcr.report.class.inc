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
 * Implements Ecxetption of AmcrReport class
 */
class AmcrReportException extends Exception {

}

/**
 * Gets report data from adobe marketing cloud
 */
class AmcrReport {

  protected $report_suite_id;
  function __construct() {
    $this->report_suite_id = 'viamtviggycom';
    $this->marketing_client = AmcrMarketingClient::getInstance();
  }

  /**
   * Get Reports data from
   */
  public function getData() {
    $period = 7*24*3600;
    $report_api = $this->marketing_client->getReportApi();
    /*
        $elements[] =  array(
          'id' => 'prop1',
          'top' => 10,
        );
        $elements[] =  array(
          'id' => 'evar61',
          'top' => 10,
        );
    */
    $elements[] =  array(
      'id' => 'domain',
      'top' => 10,
    );

    $elements[] =  array(
      'id' => 'browser',
      'top' => 10,
    );

    $elements[] =  array(
      'id' => 'ctimezone',
      'top' => 10,
    );

    $response = $report_api->queueRanked(array(
      'reportSuiteID' => $this->report_suite_id,
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
    return $insert->execute();
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
    $insert->fields(array('id', 'name', 'type', 'latency', 'current', 'list_id'));
    foreach($elements as $element) {
      $insert_data = array(
        'id' => isset($element['id']) ? $element['id'] : null,
        'name' => isset($element['name']) ? $element['name'] : null,
        'type' => isset($element['type']) ? $element['type'] : null,
        'latency' => isset($element['latency']) ? $element['latency'] : null,
        'current' => isset($element['current']) ? $element['current'] : null,
        'list_id' => $list_id,
      );
      $insert->values($insert_data);
    }
    $insert->execute();

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
    return 1;
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
    return 1;
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
  protected function saveReportDataList($data) {
    return 1;
  }
}
