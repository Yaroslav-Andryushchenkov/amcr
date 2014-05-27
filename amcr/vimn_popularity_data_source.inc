<?php
/**
 * Created by PhpStorm.
 * User: Yaraslau_Andrushchan
 * Date: 4/10/14
 * Time: 1:04 PM
 */

class VimnPopularityException extends Exception {

}



class VimnPopularityOmniture  {

  protected $marketing_client;
  protected $report_suite_id;

  function __construct() {
    $this->marketing_client = NULL;
    $this->report_suite_id = 'viamtviggycom';
    try {
      require_once drupal_get_path(
            'module',
            'amcr'
        ) . '/adobe_marketing_sdk/lib/AdobeDigitalMarketing/Autoloader.php';
      AdobeDigitalMarketing_Autoloader::register();
      $this->marketing_client = new AdobeDigitalMarketing_Client();
      $this->marketing_client->authenticate('dzemidziukp:MTV', '64ea1874aec2e447c11aee9db6b0bc07');
    } catch (Exception $e) {
      //watchdog_exception('amcr', $e);
      var_dump($e);
      die('zzz');
    }
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

    var_dump($report);

    //return $this->_get_ids($depth == 0 ? $report['report']['data'] : array_shift($report['report']['data']), $depth);
  }

}
