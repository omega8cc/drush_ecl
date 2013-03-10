<?php

/**
 * @file
 *   drush integration for caching entity content - makes most sense with
 *   entitycache module.
 *   Simple idea - load each of the entities.
 *   @TODO: Figure out how to only load entities not yet in cache.
 *   @TODO: Figure out a way to have it done via a scheduler.
 */

/**
 * Implementation of hook_drush_help().
 */
function drush_ecl_drush_help($section) {
  switch ($section) {
    case 'drush:entitycache-load':
      return dt('Used without parameters, this command loads all the various objects in the entities into cache using entity_load()');
  }
}

/**
 * Implementation of hook_drush_command().
 */
function drush_ecl_drush_command() {
  $items = array();
  $items['entitycache-load'] = array(
    'callback' => 'drush_ecl_load_cache',
    'description' => dt('Load the cache with the various entities configured to use the cache'),
    'arguments' => array(
      'type' => dt('Optional. Only load the particular entity type objects into cache'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    'aliases' => array('ecl'),
  );
  
  return $items;
}

/**
 * Load the cache bin with content from the various entities.
 * @param $type Optional. The specific type of entity that should
 *   get it's content cached.
 */
function drush_ecl_load_cache($type = '') {
  $types = entity_get_info();
  $start = time();
  if (!empty($type)) {
    if (!isset($types[$type])) {
      drush_die("Type $type is not supported");
    }
    else {
      $types = array($type => $types[$type]);
    }
  }
  
  // Lets try things in a batch.
  _drush_ecl_batch_load_cache($types);
  $batch =& batch_get();
  $batch['progressive'] = FALSE;
  drush_backend_batch_process();
}

function _drush_ecl_batch_load_cache($types) {
  if (count($types) == 0) {
    return;
  }

  $batch = array(
    'operations' => array(),
    'finished' => '_drush_ecl_batches_finished',
    'title' => dt('Loading cache...'),
    'init_message' => dt('Preparing to cache content...'),
    'progress_message' => dt('Submitting content...'),
    'error_message' => dt('Content could not get cached'),
  );
  
  foreach ($types as $entity_type => $entity_controller) {
    $batch['operations'][] = array(
      'drush_ecl_batch_load_cache', array($entity_type),
    );
  }
  batch_set($batch);
}

function _drush_ecl_batches_finished($success, $results, $operations) {
  drush_print('Succesfully cached all content!');
}

function drush_ecl_batch_load_cache($entity_type, &$context) {
  $context['message'] = dt("Begin caching $entity_type");
  _drush_ecl_load_cache($entity_type);
  $context['finished'] = TRUE;
}

/**
 * Load the cache bin with content from a specific type of entity.
 */
function _drush_ecl_load_cache($entity_type) {
  $max_limit = 50;
  $query = new EntityFieldQuery;
  $result = $query
    ->entityCondition('entity_type', $entity_type)
    ->execute();
    
  $keys = array();
  $limit = 0;
  if (count($result) <= 0) {
    return;
  }
  foreach ($result[$entity_type] as $entity_key => $entity_info) {
    $keys[] = $entity_key;
    $limit++;
    if ($limit >= $max_limit) {
      _drush_ecl_load_entities($entity_type, $keys);
      $keys = array();
      $limit = 0;
    }
  }
  // Cache any remaining keys
  _drush_ecl_load_entities($entity_type, $keys);
}

/**
 * Load the entities provided there are keys
 */
function _drush_ecl_load_entities($entity_type, $keys = array()) {
  if (count($keys) > 0) {
    entity_load($entity_type, $keys);
    entity_get_controller($entity_type)->resetCache();
  }
}
