<?php
/**
 * @file
 * Commonly used functions for dual-language editors.
 */

/**
 * Checks to see if the field should be a dual-language field or not
 *
 * @param string $entity_type
 *   The name of the bundle type
 *
 * @param string $field_key
 *   The field which we're trying to load
 *
 * @return string
 *   Returns I, N, D or H.
 *   - I: if the field is language-independent
 *   - N: if the field should not be touched
 *   - D: (default) if the field is language-dependent
 *   - H: if the field should be hidden
 */
function uottawa_lang_editor_dual_field($entity_type, $field_key) {
  // these are default keys that should not be overridden
  switch ($field_key):
    case 'actions': // the action toolbar should only exist once
    case 'form_build_id': // form build id
    case 'form_token': // form token
    case 'type': // node type should be the same
    case 'form_id': // form id
      return 'N';
    case 'translation': // we don't need the translation field
    case 'language': // we don't need the language field
      return 'H';
    case 'nid': // the NID should always be translated
    case 'vid': // the VID should always be translated
    case 'uid': // the UID should always be translated
    case 'tnid': // the TNID should always be translated (not sure if it has
    // to be)
    case 'additional_settings': // special case, should always be D
    case 'changed': // should always be D
    case 'created': // should always be D
      return 'D';
  endswitch;

  // cache the results to cut down on DB queries
  $cached = &drupal_static(__FUNCTION__, array());
  if (!isset($cached[$entity_type])) {
    // query for all the settings for this bundle at once, to cut down on
    // queries.
    $rs = db_query('
      SELECT *
      FROM {uottawa_lang_editor_field_settings}
      WHERE bundle_type = :type
    ', array(
      ':type' => $entity_type
    ));
    $cached[$entity_type] = array();
    foreach ($rs as $row) {
      $cached[$entity_type][$row->field_name] = $row->language_option;
    }
  }

  if (!isset($cached[$entity_type][$field_key])) {
    // load the default
    $field_default = variable_get('uottawa_lang_editor_entity_default');
    // the default should always be I or D.
    if ($field_default !== 'I') {
      $field_default = 'D';
    }
    // save the default to the database so that we can alter it
    db_merge('uottawa_lang_editor_field_settings')
        ->key(array('bundle_type' => $entity_type, 'field_name' => $field_key))
        ->fields(array(
          'bundle_type' => $entity_type,
          'field_name' => $field_key,
          'language_option' => $field_default,
        ))
        ->execute();
    return $field_default;
  }
  return $cached[$entity_type][$field_key];
}

/**
 * Check to see if the entity should be a dual-language form.
 *
 * @param string $entity_type
 *   The bundle name
 * @return boolean
 *   True if it should have side-by-side editing, otherwise false
 */
function uottawa_lang_editor_is_dual_entry($entity_type) {
  if (!user_access('uottawa_lang_editor_access')) {
    return FALSE;
  }
  // cache the results to cut down on DB access
  $cached = &drupal_static(__FUNCTION__, array());
  if (!isset($cached[$entity_type])) {
    $rs = db_query('
      SELECT *
      FROM {uottawa_lang_editor_bundle_settings}
      WHERE bundle_type = :type
    ', array(
      ':type' => $entity_type
    ));
    if ($rs->rowCount() > 0) {
      $row = $rs->fetchAssoc();
      $cached[$entity_type] = $row['language_option'] == 'D';
    }
    $cached[$entity_type] =
        variable_get('uottawa_lang_editor_bundle_default') !== 'I';
  }
  return $cached[$entity_type];
}
