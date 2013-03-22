<?php
/**
 * @file
 * Functions to replace the node/%/edit form (and node/create form) with a
 * side-by-side editing experience.
 */

require_once(DRUPAL_ROOT . "/modules/node/node.pages.inc");
module_load_include('inc', 'uottawa_lang_editor', 'uottawa_lang_editor.functions');

/**
 * Override function for node_form_delete() for the dual language editor.
 */
function uottawa_lang_editor_dual_language_delete($form, &$form_state) {
  // clean up form values
  $info = uottawa_lang_editor_dual_language_post_process($form, $form_state);
  // build list of nodes to delete
  $nodes = array($info['eng_state']['values']['nid'], $info['fra_state']['values']['nid']);
  // clean redirect
  header('Location: ' . url('node/delete-multiple/' . implode('-', $nodes), array(
        'absolute' => TRUE
      )));
  drupal_exit('redirecting');
}

/**
 * Override function for node_form_preview() for the dual language editor.
 */
function uottawa_lang_editor_dual_language_preview($form, &$form_state) {
  // clean up form values
  $info = uottawa_lang_editor_dual_language_post_process($form, $form_state);
  // build the english node
  $english_node = clone node_form_submit_build_node($info['eng_form'], $info['eng_state']);
  // build the french node
  $french_node = node_form_submit_build_node($info['fra_form'], $info['fra_state']);
  $preview = array(
    'leftside' => node_preview($english_node),
    'rightside' => node_preview($french_node)
  );
  if (module_exists('superpreview')) {
    $popup_id = superpreview_begin_preview(array(
      'en' => array(
        '#content' => $preview['leftside'],
        '#title' => $info['eng_state']['values']['title'],
      ),
      'fr' => array(
        '#content' => $preview['rightside'],
        '#title' => $info['fra_state']['values']['title'],
      ),
    ));
    $form_state['node_preview'] = $popup_id;
  }
  else {
    $form_state['node_preview'] = theme('side_by_side', $preview);
  }
  $form_state['force_duallang'] = TRUE;
  // we want to show the form again.
  $form_state['rebuild'] = TRUE;
}

/**
 * Compares values from two objects, recursively if needed.
 *
 * Note that one key difference between this and == is that
 *   this only checks that all the keys on the leftSide arrays
 *   are the same on the rightSide. It does not go the other way
 *   (ie the rightSide object can declare fields that the leftSide
 *   does not.
 *
 * @param mixed $left
 *   First object
 *
 * @param mixed $right
 *   Second object
 *
 * @return boolean
 *   True if the $rightSide object has all the same info as the $leftSide
 *   object.
 */
function uottawa_lang_editor_value_compare($left, $right) {
  if (is_array($left)) {
    if (is_array($right)) {
      // if both are arrays, recurse for each key
      foreach ($left as $key => $val) {
        if (isset($right[$key])) {
          if (!uottawa_lang_editor_value_compare($val, $right[$key])) {
            return FALSE;
          }
        }
      }
      return TRUE;
    }
    else {
      return FALSE;
    }
  }
  else {
    return $left == $right;
  }
}

/**
 * Checks for any differences between the old node and the new information.
 *
 * @param string $language
 *   The language we are checking
 *
 * @return boolean
 *   True if there are differences, otherwise false
 */
function uottawa_lang_editor_check_for_changes($form, $form_state, $language = 'en') {
  $info = uottawa_lang_editor_dual_language_post_process($form, $form_state);
  $state = ($language == 'fr' ? $info['fra_state'] : $info['eng_state']);
  // create a new node based on the values
  $new_node = (object) $state['values'];
  if (!isset($new_node->nid)) {
    // new nodes (ie that have no NID) are always going to have changes
    return TRUE;
  }
  // load the old node
  $old_node = node_load($new_node->nid);
  // remove fields that we don't want to count as 'differences'
  if (isset($old_node->changed)) {
    unset($old_node->changed);
  }
  if (isset($old_node->created)) {
    unset($old_node->created);
  }
  if (isset($old_node->revision)) {
    unset($old_node->revision);
  }
  if (isset($old_node->log)) {
    unset($old_node->log);
  }
  // The menu is a bit odd, this is a bit of a hack
  if (isset($old_node->menu) && isset($old_node->menu['description'])) {
    $old_node->menu['description'] = $old_node->menu['localized_options']['attributes']['title'];
  }
  // Compare the values
  foreach ($old_node as $key => $value) {
    if (isset($new_node->$key)) {
      if (!uottawa_lang_editor_value_compare($value, $new_node->$key)) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

/**
 * Custom validation for a dual-language form.
 */
function uottawa_lang_editor_dual_language_form_my_validation($form, &$form_state) {
  // Y means we force revisions on save; S means we force revisions for both
  // or none (but not one); N means we do nothing.
  $rev_setting = variable_get('uottawa_lang_editor_linked_revisions');
  if ($rev_setting == 'Y') {
    // force a revision to be done
    $form_state['values']['revision'] = '1';
    $form_state['values']['revision_french'] = '1';
  }
  elseif ($rev_setting == 'S') {
    // if either is being revised, force a revision
    if ($form_state['values']['revision'] == '1' || $form_state['values']['revision_french'] == '1') {
      $form_state['values']['revision'] = '1';
      $form_state['values']['revision_french'] = '1';
    }
  }
  // Y means we force a comment from the user, D means to use default text,
  // N means we can allow blank comments
  $log_setting = variable_get('uottawa_lang_editor_revision_comments');
  if ($log_setting == 'Y') {
    // error if there is no log and we are in a revision
    if ($form_state['values']['revision'] == '1' && $form_state['values']['log'] == '') {
      form_set_error('log', t('You must enter a revision log entry for the English content'));
    }
    // error if there is no log and we are in a revision, for french
    if ($form_state['values']['revision_french'] == '1' && $form_state['values']['log_french'] == '') {
      form_set_error('log_french', t('You must enter a revision log entry for the French content'));
    }
  }
  elseif ($log_setting == 'D') {
    if ($form_state['values']['revision'] == '1' && $form_state['values']['log'] == '') {
      // creating a new node
      if ($form_state['values']['nid'] < 1) {
        $form_state['values']['log'] = 'LAC:: ' . variable_get('uottawa_lang_editor_comment_creation');
        // the node has been changed
      }
      elseif (uottawa_lang_editor_check_for_changes($form, $form_state, 'en')) {
        $form_state['values']['log'] = 'LAM:: ' . variable_get('uottawa_lang_editor_comment_minor_changes');
        // the node has not been changed
      }
      else {
        $form_state['values']['log'] = 'LAN:: ' . variable_get('uottawa_lang_editor_comment_no_changes');
      }
    }
    if ($form_state['values']['revision_french'] == '1' && $form_state['values']['log_french'] == '') {
      // creating a new node
      if ($form_state['values']['nid_french'] < 1) {
        $form_state['values']['log_french'] = 'LAC:: ' . variable_get('uottawa_lang_editor_comment_creation');
        // the node has been changed
      }
      elseif (uottawa_lang_editor_check_for_changes($form, $form_state, 'fr')) {
        $form_state['values']['log_french'] = 'LAM:: ' . variable_get('uottawa_lang_editor_comment_minor_changes');
        // the node has not been changed
      }
      else {
        $form_state['values']['log_french'] = 'LAN:: ' . variable_get('uottawa_lang_editor_comment_no_changes');
      }
    }
  }
}

/**
 * Override function for node_form_validate() for the dual language editor.
 */
function uottawa_lang_editor_dual_language_form_validate($form, &$form_state) {
  module_load_include('inc', 'uottawa_lang_editor', 'uottawa_lang_editor.theme');
  uottawa_lang_editor_include_side_by_side_css();
  uottawa_lang_editor_dual_language_form_my_validation($form, $form_state);
  $info = uottawa_lang_editor_dual_language_post_process($form, $form_state);
  // validate french components
  node_form_validate($info['fra_form'], $info['fra_state']);
  // validate english components
  node_form_validate($info['eng_form'], $info['eng_state']);
}

/**
 * Override function for node_form_submit() for the dual language editor.
 */
function uottawa_lang_editor_dual_language_submit($form, &$form_state) {
  global $language;
  $info = uottawa_lang_editor_dual_language_post_process($form, $form_state);
  // create/update the english form
  $state = $info['eng_state'];
  $state['values']['language'] = 'en';
  node_form_submit($info['eng_form'], $state);
  $english_nid = $state['nid'];
  $english_node = node_load($english_nid);
  $english_node->language = 'en';
  $english_node->path = array(
    'pid' => '',
    'source' => '',
    'alias' => '',
    'language' => 'en'
  );
  if (isset($english_node->nid)) {
    // find the appropriate alias
    $rs = db_query_range('
      SELECT *
      FROM {url_alias}
      WHERE source = :source AND language = :lang
      ORDER BY pid ASC', 0, 1, array(
      ':source' => 'node/' . $english_node->nid,
      ':lang' => 'en'
    ));
    if ($rs->rowCount() > 0) {
      $english_node->path = $rs->fetchAssoc();
    }
    $english_node->path['pathauto'] = FALSE;
    module_load_include('inc', 'pathauto');
    $autoalias = pathauto_create_alias('node', 'return', 'node/' . $english_node->nid, array('node' => $english_node), $english_node->type, 'en');
    if ($autoalias != 'node/' . $english_node->nid) {
      if ($autoalias == $english_node->path['alias']) {
        $english_node->path['pathauto'] = TRUE;
      }
    }
  }
  $english_node->tnid = $english_node->nid;
  node_save($english_node);
  // create/update the french form
  $info['fra_state']['values']['language'] = 'fr';
  $info['fra_state']['values']['tnid'] = $english_nid;
  $fstate = $info['fra_state'];
  node_form_submit($info['fra_form'], $fstate);
  if ($state['values']['revision'] == '1' && $fstate['values']['revision'] == '1') {
    $french_node = node_load($fstate['nid']);
    $french_nid = $french_node->nid;
    $french_vid = $french_node->vid;
    $english_vid = $english_node->vid;
    db_insert('uottawa_lang_editor_revision_links')
        ->fields(array(
          'english_nid' => $english_nid,
          'french_nid' => $french_nid,
          'english_vid' => $english_vid,
          'french_vid' => $french_vid
        ))->execute();
  }
  unset($form_state['redirect']);
  if (isset($_GET['destination'])) {
    unset($_GET['destination']);
  }
  if ($language->language == 'en') {
    $form_state['redirect'] = 'node/' . $english_node->nid;
  }
  else {
    $form_state['redirect'] = 'node/' . $french_node->nid;
  }
}

/**
 * Converts all french keys from an array back to their original form.
 *
 * @param mixed $french_vals
 *   The array to remove tokens from the keys. If not an array, then will be
 *   returned without being changed.
 *
 * @return mixed
 *   The value passed, with the array keys cleaned up if it's an array.
 */
function uottawa_lang_editor_clean_language_tokens($french_vals) {
  if (!is_array($french_vals)) {
    // if we're not processing an array for it's keys, just return the value.
    return $french_vals;
  }
  foreach ($french_vals as $key => $value) {
    // form values are never frenchified. Ignore them.
    // this is not 100% true (#parents and #group are frenchified) but they don't really matter.
    if (drupal_substr($key, 0, 1) != '#') {
      if (drupal_strlen($key) > 7) {
        if (drupal_substr($key, -7) == '_french') {
          // insert a new value for the cleaned up element
          $french_vals[drupal_substr($key, 0, -7)] = uottawa_lang_editor_clean_language_tokens($value);
          // remove the french element
          unset($french_vals[$key]);
        }
      }
    }
  }
  return $french_vals;
}

/**
 * Separates the $form and $form_state variables into their appropriate
 * English and French forms.
 *
 * @return array
 *   An array with four keys:
 *   - eng_form: The English $form array
 *   - fra_form: The French $form array
 *   - eng_state: The English $form_state array
 *   - fra_state: The French $form_state array
 */
function uottawa_lang_editor_dual_language_post_process($form, $form_state) {
  // Initialize values
  $data = array(
    'eng_form' => array(),
    'fra_form' => array(),
    'eng_state' => array(),
    'fra_state' => array()
  );
  $french_values = array();
  // look for frenchified keys
  foreach ($form_state['values'] as $key => $value) {
    if (drupal_strlen($key) > 7) {
      if (drupal_substr($key, -7) == '_french') {
        // store them for the french form_state['values'] list
        $french_values[$key] = $value;
        // remove them from the English list
        unset($form_state['values'][$key]);
      }
    }
  }
  // clean up the french tokens
  $french_values = uottawa_lang_editor_clean_language_tokens($french_values);
  // the original form is stored in this key, let's just use it
  $data['eng_form'] = $form_state['uottawa_lang_editor_original_form'];
  $data['fra_form'] = $form_state['uottawa_lang_editor_original_form'];
  // the english state is the state without the french keys we removed above
  $data['eng_state'] = $form_state;
  // make sure the english form is set to English
  $data['eng_state']['values']['language'] = 'en';
  unset($data['eng_state']['node_french']);
  // the base of the French state is the same as the English state
  $data['fra_state'] = $form_state;
  // but we need to update the values array to have the french values
  $data['fra_state']['values'] = array('language' => 'fr') + $french_values + $form_state['values'];
  $data['fra_state']['node'] = $form_state['node_alt'];
  if ($data['fra_state']['node']->language == 'en') {
    $data['fra_state']['node'] = $form_state['node'];
    $data['eng_state']['node'] = $form_state['node_alt'];
  }
  return $data;
}

/**
 * Updates $node to be ready for inserting.
 *
 * @param object $node
 *   The node to update.
 * @param string $language
 *   The language of the node to load
 */
function uottawa_lang_editor_extend_node(&$node, $language) {
  $node->language = $language;
  // default values for path
  $node->path = array(
    'pid' => '',
    'source' => '',
    'alias' => '',
    'language' => $language
  );
  if (isset($node->nid)) {
    // find the appropriate alias
    $rs = db_query_range('
      SELECT *
      FROM {url_alias}
      WHERE source = :source AND language = :lang
      ORDER BY pid ASC
    ', 0, 1, array(
      ':source' => 'node/' . $node->nid,
      ':lang' => $language
    ));
    if ($rs->rowCount() > 0) {
      $node->path = $rs->fetchAssoc();
    }
  }
  // Run other preparations on the object.
  node_object_prepare($node);
}

/**
 * Create the dual-language form.
 *
 * @param string $entity_type
 *   The entity we'll be editing.
 */
function uottawa_lang_editor_dual_language_form(&$form, &$form_state, $entity_type) {
  if (!uottawa_lang_editor_is_dual_entry($entity_type)) {
    return;
  }
  // these will hold our two forms
  $english_form = NULL;
  $french_form = NULL;
  $original_form = $form;
  $form_state['uottawa_lang_editor_original_form'] = $original_form;
  // we need to know if we're creating a new item or not
  $is_new = empty($form_state['build_info']['args'][0]->nid);
  // for the french form
  if ($form_state['build_info']['args'][0]->language == 'fr') {
    $french_form = $form;
    $english_node = NULL;
    if ($is_new) {
      // copy the new node as an English node
      $english_node = clone $form_state['build_info']['args'][0];
      $english_node->language = 'en';
    }
    else {
      // find and load the English node
      if (!empty($form_state['build_info']['args'][0]->tnid)) {
        $english_node = node_load($form_state['build_info']['args'][0]->tnid, NULL, TRUE);
      }
    }
    // no english node = warning message
    if (empty($english_node)) {
      drupal_set_message(t('Could not load matching node'), 'warning');
      return;
    }
    // extend the node to have all it's properties
    uottawa_lang_editor_extend_node($english_node, 'en');
    $form_state['node_alt'] = $english_node;
    // create the alt language form
    $english_form = uottawa_lang_editor_load_alt_form($english_node);
  }
  // for the english form
  else {
    $english_form = $form;
    $french_node = NULL;
    if ($is_new) {
      // copy the new node as the French node
      $french_node = clone $form_state['build_info']['args'][0];
      $french_node->language = 'fr';
    }
    else {
      // find and load the French node
      if (!empty($form_state['build_info']['args'][0]->tnid)) {
        $translations = translation_node_get_translations($form_state['build_info']['args'][0]->tnid);
        if (!empty($translations['fr'])) {
          $french_node = node_load($translations['fr']->nid, NULL, TRUE);
        }
      }
    }
    // no french node = warning message
    if (empty($french_node)) {
      drupal_set_message(t('Could not load matching node'), 'warning');
      return;
    }
    // extend the node to have all it's properties
    uottawa_lang_editor_extend_node($french_node, 'fr');
    $form_state['node_alt'] = $french_node;
    // create the alt language form
    $french_form = uottawa_lang_editor_load_alt_form($french_node);
  }
  module_load_include('inc', 'uottawa_lang_editor', 'uottawa_lang_editor.theme');
  uottawa_lang_editor_include_side_by_side_css();
  $form['english_block'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('uottawa_lang_editor_english', 'uottawa_lang_editor_block')),
    '#weight' => -5001,
    'markup' => array(
      '#markup' => '<h2>' . t('English Content') . '</h2><p class="uottawa_lang_editor_actions"><span id="uottawa_lang_editor_collapse_english" class="uottawa_lang_editor_collapse">' . t('Collapse') . '</span><span id="uottawa_lang_editor_expand_english" class="uottawa_lang_editor_expand">' . t('Expand') . '</span></p>',
      '#weight' => -5000000,
    ),
  );
  // This container will store all the French components
  $form['french_block'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('uottawa_lang_editor_french', 'uottawa_lang_editor_block')),
    '#weight' => -5001,
    'markup' => array(
      '#markup' => '<h2>' . t('French Content') . '</h2><p class="uottawa_lang_editor_actions"><span id="uottawa_lang_editor_collapse_french" class="uottawa_lang_editor_collapse">' . t('Collapse') . '</span><span id="uottawa_lang_editor_expand_french" class="uottawa_lang_editor_expand">' . t('Expand') . '</span></p>',
      '#weight' => -5000000,
    ),
  );
  // Spacing div between English/French and the Shared block
  $form['spacer_block'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('uottawa_lang_editor_spacer', 'uottawa_lang_editor_block')),
    '#weight' => -5000,
  );
  // Container to store all the shared components
  $form['independent_block'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('uottawa_lang_editor_independent', 'uottawa_lang_editor_block')),
    '#weight' => -4999,
    'markup' => array(
      '#markup' => '<h2>' . t('Shared Settings') . '</h2>',
      '#weight' => -5000000,
    ),
  );
  foreach (array_keys($original_form) as $form_key) {
    if (drupal_substr($form_key, 0, 1) != '#') {
      switch (uottawa_lang_editor_dual_field($entity_type, $form_key)) {
        case 'I':
          $form['independent_block'][$form_key] = $original_form[$form_key];
          unset($form[$form_key]);
          break;
        case 'N':
          break;
        case 'H':
          $value = '';
          if (isset($original_form[$form_key]['#value'])) {
            $value = $original_form[$form_key]['#value'];
          } else if (isset($original_form[$form_key['#default_value']])) {
            $value = $original_form[$form_key]['#default_value'];
          }
          $form['independent_block'][$form_key] = array(
            '#type' => 'value',
            '#value' => $value,
          );
          unset($form[$form_key]);
          break;
        default:
        case 'D':
          $new_key = $form_key;
          if ($form_key == 'additional_settings') {
            $new_key = 'additional_settings_english';
          }
          $form['english_block'][$new_key] = $english_form[$form_key];
          if (!empty($form['english_block'][$form_key]['#group'])) {
            $form['english_block'][$form_key]['#group'] .= '_english';
          }
          $form['french_block'][$form_key . '_french'] =
              uottawa_lang_editor_translate_keys($english_form[$form_key], $french_form[$form_key], $form_key);
          if ($form_key != 'additional_settings') {
            unset($form[$form_key]);
          }
      }
    }
  }
  $form['#validate'] = array('uottawa_lang_editor_dual_language_form_validate');
  $form['actions']['submit']['#submit'] = array('uottawa_lang_editor_dual_language_submit');
  $form['actions']['preview']['#submit'] = array('uottawa_lang_editor_dual_language_preview');
  $form['actions']['delete']['#submit'] = array('uottawa_lang_editor_dual_language_delete');
  if (isset($form['actions']['preview_changes'])) {
    $form['actions']['preview_changes']['#submit'] = array('uottawa_lang_editor_dual_language_changes');
  }
}

/**
 * Form submit to show the changes between last saved and current versions.
 */
function uottawa_lang_editor_dual_language_changes($form, &$form_state) {
  $info = uottawa_lang_editor_dual_language_post_process($form, $form_state);
  $eng_state = $info['eng_state'];
  $fra_state = $info['fra_state'];
  diff_node_form_build_preview_changes($info['eng_form'], $eng_state);
  diff_node_form_build_preview_changes($info['fra_form'], $fra_state);
  $form_state['node_preview'] = theme('side_by_side', array(
    'leftside' => $eng_state['node_preview'],
    'rightside' => $fra_state['node_preview']
  ));
  $form_state['force_duallang'] = TRUE;
  $form_state['rebuild'] = TRUE;
}

/**
 * A custom implementation of path_form_element_validate explicitly for the
 * French version.
 *
 * @param array $element
 *   The element to validate
 *
 * @param array $form_state
 *   The form state
 *
 * @param array $complete_form
 *   The complete form
 */
function uottawa_lang_editor_french_path_form_element_validate($element, &$form_state, $complete_form) {
  // changed values to the _french values as that is what we have at this time
  $alias = trim($form_state['values']['path_french']['alias_french']);
  if (!empty($alias)) {
    form_set_value($element['alias_french'], $alias, $form_state);
    // also make sure the language is always french
    form_set_value($element['language_french'], 'fr', $form_state);
    $path = $form_state['values']['path_french'];
    $query = db_select('url_alias')
        ->condition('alias', $path['alias_french'])
        ->condition('language', $path['language_french']);
    if (!empty($path['source_french'])) {
      $query->condition('source', $path['source_french'], '<>');
    }
    $query->addExpression('1');
    $query->range(0, 1);
    if ($query->execute()->fetchField()) {
      form_error($element, t('The alias is already in use.'));
    }
  }
}

/**
 * Translates the keys of a form entry recursively to include _french at the
 * end.
 *
 * @param array $parent
 *   The form to translate.
 *
 * @param array $form
 *   The original french form to take values from (must be similar to $parent).
 *
 * @param string $parent_key
 *   The $key of the parent element (one up).
 *
 * @param string $super_key
 *   The $key of the parent element two levels up.
 *
 * @return array
 *   Returns translated version of $parent (or $parent itself if an array
 *   wasn't passed).
 */
function uottawa_lang_editor_translate_keys($parent, $form, $parent_key, $super_key = '') {
  if (is_array($parent)) {
    $new_parent = array();
    foreach ($parent as $key => $value) {
      // These keys indicate we need to get a value from the french version of
      // the form
      if ($key == '#default_value' || $key == '#value' || $key == 'value' || $key == '#options') {
        if (!empty($form[$key])) {
          $new_parent[$key] = $form[$key];
        } else {
          $new_parent[$key] = '';
        }
      }
      // if it doesn't begin with #, it is likely a field, we should process it
      elseif (drupal_substr($key, 0, 1) != '#') {
        if (isset($form[$key])) {
          $new_parent[$key . '_french'] = uottawa_lang_editor_translate_keys($value, $form[$key], $key, $parent_key);
        } else {
          $new_parent[$key . '_french'] = uottawa_lang_editor_translate_keys($value, array(), $key, $parent_key);
        }
      }
      // #groups need _french appended
      elseif ($key == '#group') {
        $new_parent[$key] = $value . '_french';
      }
      // #parents need to have _french appended to each of them
      elseif ($key == '#parents') {
        $list = array();
        foreach ($value as $item) {
          $list[] = $item . '_french';
        }
        $new_parent[$key] = $list;
      }
      // we need to check if any validation elements need to be replaced
      elseif ($key == '#element_validate') {
        $new_parent[$key] = UOttawaLanguageEditor_FrenchCleanup::i()->cleanup_validation_tokens($value);
      }
      // we need to check if any states need to be replaced
      elseif ($key == '#states') {
        $new_values = array();
        foreach ($value as $state => $keys) {
          $new_values[$state] = UOttawaLanguageEditor_FrenchCleanup::i()->cleanup_state_tokens($keys);
        }
        $new_parent[$key] = $new_values;
      }
      // we need to check if any classes need to be replaced
      elseif ($key == '#attributes') {
        if (isset($value['class'])) {
          $value['class'] = UOttawaLanguageEditor_FrenchCleanup::i()->cleanup_class_tokens($value['class']);
        }
        $new_parent[$key] = $value;
      }
      // we need to check if any js files need to be replaced
      elseif ($key == '#attached') {
        $new_value = $value;
        if (isset($value['js'])) {
          $new_value['js'] = UOttawaLanguageEditor_FrenchCleanup::i()->cleanup_js_tokens($value['js']);
        }
        $new_parent['#attached'] = $new_value;
      }
      // otherwise just keep the value
      else {
        $new_parent[$key] = $value;
      }
    }
    return $new_parent;
  }
  return $parent;
}

/**
 * Obtains the french version of the form to create a node.
 *
 * This is basically a copy of the Drupal method, but tweaked to add a few
 * things.
 *
 * @param object $node
 *   The node to create.
 *
 * @return array
 *   The $form for the French edit page.
 */
function uottawa_lang_editor_load_alt_form($node) {
  $form_state = array('storage' => NULL, 'submitted' => FALSE);
  $form_state += form_state_defaults();
  $form_state['build_info']['args'] = array(
    $node
  );
  // Flag variable to prevent this module from recursively looping
  $form_state['uottawa_lang_editor_norebuild'] = 'yes';

  $form_id = $node->type . '_node_form';

  $languages = language_list();
  $old_content = clone $GLOBALS['language_content'];
  $GLOBALS['language_content'] = $languages[$node->language];

  drupal_static_reset('taxonomy_get_tree');
  drupal_static_reset('taxonomy_get_tree:parents');
  drupal_static_reset('taxonomy_get_tree:terms');
  $form = drupal_retrieve_form($form_id, $form_state, $node);
  drupal_prepare_form($form_id, $form, $form_state);

  $GLOBALS['language_content'] = $old_content;

  return $form;
}

/**
 * Override of node_add to force English content.
 */
function uottawa_lang_editor_node_add($type) {
  module_load_include('inc', 'uottawa_lang_editor',
      'uottawa_lang_editor.functions');
  module_load_include('inc', 'node', 'node.pages');
  if (!uottawa_lang_editor_is_dual_entry($type)) {
    return node_add($type);
  }

  $languages = language_list();
  $old_content = clone $GLOBALS['language_content'];
  $GLOBALS['language_content'] = $languages['en'];

  drupal_static_reset('taxonomy_get_tree');
  drupal_static_reset('taxonomy_get_tree:parents');
  drupal_static_reset('taxonomy_get_tree:terms');

  $result = node_add($type);

  $GLOBALS['language_content'] = $old_content;

  return $result;
}

/**
 * Override of node_edit().
 */
function uottawa_lang_editor_node_edit($node) {
  module_load_include('inc', 'uottawa_lang_editor',
      'uottawa_lang_editor.functions');
  module_load_include('inc', 'node', 'node.pages');
  if (!uottawa_lang_editor_is_dual_entry($node->type)) {
    return node_page_edit($node);
  }

  $languages = language_list();
  $old_content = clone $GLOBALS['language_content'];
  $GLOBALS['language_content'] = $languages['en'];

  drupal_static_reset('taxonomy_get_tree');
  drupal_static_reset('taxonomy_get_tree:parents');
  drupal_static_reset('taxonomy_get_tree:terms');

  $result = node_page_edit($node);

  $GLOBALS['language_content'] = $old_content;

  return $result;
}
