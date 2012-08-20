<?php
/**
 * @file
 * Functions related to the Features implementation.
 */

/**
 * Implements hook_features_export_options().
 */
function uottawa_lang_editor_bundle_config_features_export_options() {
  $options = array();
  $rs = db_query('SELECT * FROM {uottawa_lang_editor_bundle_settings}');
  foreach ($rs as $row) {
    $options[$row->bundle_type] = $row->bundle_type;
  }
  return $options;
}

/**
 * Implements hook_features_export().
 */
function uottawa_lang_editor_bundle_config_features_export($data, &$export, $module_name) {
  $export['dependencies']['uottawa_lang_editor'] = 'uottawa_lang_editor';
  foreach ($data as $bundle) {
    $export['features']['uottawa_lang_editor_bundle_config'][$bundle] = $bundle;
  }
}

/**
 * Implements hook_features_export_render().
 */
function uottawa_lang_editor_bundle_config_features_export_render($module_name, $data, $export = NULL) {
  $rs = db_query('SELECT * FROM {uottawa_lang_editor_bundle_settings}');
  $bundle_info = array();
  foreach ($rs as $row) {
    $bundle_info[$row->bundle_type] = (array) $row;
    unset($bundle_info[$row->bundle_type]['pkid']);
  }

  $rs = db_query('SELECT * FROM {uottawa_lang_editor_field_settings}');
  $field_info = array();
  foreach ($rs as $row) {
    if (!isset($field_info[$row->bundle_type])) {
      $field_info[$row->bundle_type] = array();
    }
    $field_info[$row->bundle_type][$row->field_name] = (array) $row;
    unset($field_info[$row->bundle_type][$row->field_name]['pkid']);
  }

  $code = array();
  $code[] = '  $bundles = array();';
  $code[] = '';
  foreach ($data as $bundle) {
    $package = array(
      'bundle' => $bundle_info[$bundle],
      'fields' => $field_info[$bundle]
    );
    $code[] = '  $bundles[] = ' . features_var_export($package, '  ') . ';';
  }
  $code[] = '  return $bundles;';
  return array('config_features_bundle_default_settings' => implode('\n', $code));
}

/**
 * Implements hook_features_rebuild();
 */
function uottawa_lang_editor_bundle_config_features_rebuild($module) {
  $items = module_invoke($module, 'config_features_bundle_default_settings');
  foreach ($items as $item) {
    $bundle = $item['bundle'];
    db_merge('uottawa_lang_editor_bundle_settings')
        ->fields($bundle)
        ->key(array('bundle_type' => $bundle['bundle_type']))
        ->execute();
    foreach ($item['fields'] as $field) {
      db_merge('uottawa_lang_editor_field_settings')
          ->fields($field)
          ->key(array(
            'bundle_type' => $field['bundle_type'],
            'field_name' => $field['field_name']
          ))
          ->execute();
    }
  }
}

/**
 * Implements hook_features_revert().
 */
function uottawa_lang_editor_config_features_revert($module) {
  uottawa_lang_editor_bundle_config_features_rebuild($module);
}