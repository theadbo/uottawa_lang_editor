<?php

/**
 * @file
 * Configuration controls for this module.
 */

/**
 * Form constructor for the config form.
 */
function uottawa_lang_editor_admin_form($form, &$form_state) {
  $form = array();
  $form['configuration'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#title' => 'Base Configuration',
    'uottawa_lang_editor_bundle_default' => array(
      '#type' => 'select',
      '#title' => 'Translate Types By Default?',
      '#default_value' =>
          variable_get('uottawa_lang_editor_bundle_default', 'D'),
      '#options' => array(
        'D' => 'Yes',
        'I' => 'No'
      )
    ),
    'uottawa_lang_editor_field_default' => array(
      '#type' => 'select',
      '#title' => 'Translate Fields By Default?',
      '#default_value' =>
          variable_get('uottawa_lang_editor_field_default', 'D'),
      '#options' => array(
        'D' => 'Yes',
        'I' => 'No'
      )
    )
  );
  $form['revision_configuration'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#title' => 'Revision Control',
    'uottawa_lang_editor_linked_revisions' => array(
      '#type' => 'select',
      '#title' => 'Linked Revisions',
      '#default_value' =>
          variable_get('uottawa_lang_editor_linked_revisions', 'S'),
      '#options' => array(
        'N' => 'No Link Between Languages',
        'S' => 'Force Link Between Languages',
        'Y' => 'Force Revision on Save'
      )
    ),
    'uottawa_lang_editor_revision_comments' => array(
      '#type' => 'select',
      '#title' => 'Revision Log',
      '#default_value' =>
          variable_get('uottawa_lang_editor_revision_comments', 'D'),
      '#options' => array(
        'N' => 'Allow blank log entries',
        'D' => 'Provide default text for log entries',
        'Y' => 'Force entry of log entries'
      )
    ),
    'uottawa_lang_editor_comment_no_changes' => array(
      '#type' => 'textarea',
      '#title' => 'Default Log Text (No Changes)',
      '#default_value' => variable_get('uottawa_lang_editor_comment_no_changes',
          'No changes / Pas de changements'),
      '#states' => array(
        'visible' => array(
          ':input[name="uottawa_lang_editor_revision_comments"]' =>
              array('value' => 'D'),
        )
      )
    ),
    'uottawa_lang_editor_comment_minor_changes' => array(
      '#type' => 'textarea',
      '#title' => 'Default Log Text (Minor Changes)',
      '#default_value' =>
          variable_get('uottawa_lang_editor_comment_minor_changes',
              'Minor changes / Des changements mineurs'),
      '#states' => array(
        'visible' => array(
          ':input[name="uottawa_lang_editor_revision_comments"]' =>
              array('value' => 'D'),
        )
      )
    ),
    'uottawa_lang_editor_comment_creation' => array(
      '#type' => 'textarea',
      '#title' => 'Default Log Text (Creation)',
      '#default_value' => variable_get('uottawa_lang_editor_comment_creation',
          'Content was created / Le contenu a été créé'),
      '#states' => array(
        'visible' => array(
          ':input[name="uottawa_lang_editor_revision_comments"]' =>
              array('value' => 'D'),
        )
      )
    ),
    'uottawa_lang_editor_comment_revert' => array(
      '#type' => 'textarea',
      '#title' => 'Log Text (Revert)',
      '#default_value' => variable_get('uottawa_lang_editor_comment_revert',
          'Revision copy / Copie de révision [!timestamp]'),
      '#description' =>
          t('Use "!timestamp" to insert the date/time of the revision',
              array('!timestamp' => '!timestamp')),
    )
  );
  $form['revision_removal'] = array(
    '#title' => t('Revision Expiry'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    'uottawa_lang_editor_decay_minor' => array(
      '#title' => t('Minor Revision Expiry'),
      '#description' =>
          t('The number of seconds after which a minor revision will expire.' .
              ' Enter 0 or less to prevent them from ever expiring.'),
      '#type' => 'textfield',
      '#element_validate' => array('_uottawa_lang_editor_validate_seconds'),
      '#default_value' => variable_get('uottawa_lang_editor_decay_minor', 0),
    ),
    'uottawa_lang_editor_decay_major' => array(
      '#title' => t('Major Revision Expiry'),
      '#description' =>
          t('The number of seconds after which a major revision will expire.' .
              ' Enter 0 or less to prevent them from ever expiring.'),
      '#type' => 'textfield',
      '#element_validate' => array('_uottawa_lang_editor_validate_seconds'),
      '#default_value' => variable_get('uottawa_lang_editor_decay_major', 0),
    ),
    'uottawa_lang_editor_decay_auto' => array(
      '#title' => t('Auto-Remove'),
      '#description' => t('Automatically remove older revisions'),
      '#type' => 'select',
      '#options' => array(
        'Y' => 'Yes',
        'N' => 'No'
      ),
      '#default_value' => variable_get('uottawa_lang_editor_decay_auto', 'N'),
    ),
    'uottawa_lang_editor_decay_both' => array(
      '#title' => t('Linked Deletions'),
      '#type' => 'select',
      '#options' => array(
        'L' => 'Delete only when both revisions are expired',
        'A' => 'Delete both revisions when either is expired',
        'O' => 'Delete expired revisions, preserve linked revisions',
      ),
      '#default_value' => variable_get('uottawa_lang_editor_decay_both', 'L'),
    ),
  );
  return system_settings_form($form);
}

/**
 * Validation callback for uottawa_lang_editor_admin_form().
 */
function _uottawa_lang_editor_validate_seconds($element, &$form_state, $form) {
  if (!isset($element["#value"])) {
    $element["#value"] = 0;
  }
  if (!filter_var($element["#value"], FILTER_VALIDATE_INT)) {
    form_set_error($element["#name"], t('This field must be an integer'));
  }
}

/**
 * Form constructor for the revisions form.
 */
function uottawa_lang_editor_old_revisions_form($form, &$form_state) {
  $form = array();
  $headers = array(
    'content' => array('data' => 'Content'),
    'revision' => array('data' => 'Revision Date/Time'),
    'log' => array('data' => 'Log Entry'),
  );
  $options = uottawa_lang_editor_get_old_revisions();

  $form['revisions'] = array(
    '#type' => 'tableselect',
    '#header' => $headers,
    '#options' => $options,
    '#empty' => t('No revisions meet the criteria'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Delete Selected',
    '#submit' => array('uottawa_lang_editor_old_revisions_form_submit'),
  );
  return $form;
}

/**
 * Submit handler for uottawa_lang_editor_old_revisions_form().
 */
function uottawa_lang_editor_old_revisions_form_submit($form, &$form_state) {
  $delete_ids = array();
  foreach ($form_state["values"]["revisions"] as $key => $value) {
    if ($value !== 0) {
      $parts = explode("-", $value);
      $node = new stdClass();
      $node->nid = $parts[0];
      $node->vid = $parts[1];
      $delete_ids[$node->vid] = $node;
    }
  }
  uottawa_lang_editor_old_revisions_delete($delete_ids);
  drupal_set_message(t('The selected revisions have been removed'));
}

/**
 * Formats a line for a revision entry in the deletion list.
 *
 * @param object $row
 *   A revision entry.
 *
 * @return array
 *   A row suitable for use in a tableselect form element.
 */
function uottawa_lang_editor_get_revision_line($row) {
  $rev = '';
  $rev = format_date($row->timestamp);
  $log = $row->log;
  if (drupal_strlen($log) > 5) {
    // we need to remove the log prefixes
    switch (drupal_substr($log, 0, 5)) {
      case 'LAM::':
      case 'LAN::':
      case 'LAC::':
        $log = trim(drupal_substr($log, 5));
    }
  }
  return array(
    'content' => $row->title,
    'revision' => $rev,
    'log' => $log
  );
}

/**
 * Ensures that a list of items to delete contains all the items it should
 * and no others.
 *
 * @param array $list
 *   An array of node objects with just nid and vid populated.
 *
 * @return array
 *   An updated array of node objects.
 */
function uottawa_lang_editor_old_revisions_sanitize_deletions($list) {
  $mode = variable_get('uottawa_lang_editor_decay_both');
  switch ($mode) {
    case 'O':
      // we can delete just the expired versions, return the list
      return $list;
    case 'A':
      // we should make sure we delete the linked version as well
      foreach ($list as $item) {
        $rs = db_query("
          SELECT *
          FROM {uottawa_lang_editor_revision_links}
          WHERE english_vid = :vid OR french_vid = :vid
        ", array(
          ':vid' => $item->vid
        ));
        foreach ($rs as $row) {
          $en = new stdClass();
          $en->vid = $row->english_vid;
          $en->nid = $row->english_nid;
          $list[$en->vid] = $en;
          $fr = new stdClass();
          $fr->vid = $row->french_vid;
          $fr->nid = $row->french_nid;
          $list[$fr->vid] = $fr;
        }
      }
    case 'L':
      // we should delete this item only if we're deleting both items
      foreach ($list as $item) {
        $rs = db_query("
          SELECT *
          FROM {uottawa_lang_editor_revision_links}
          WHERE english_vid = :vid OR french_vid = :vid
        ", array(
          ':vid' => $item->vid
        ));
        foreach ($rs as $row) {
          $en_check = isset($list[$row->english_vid]);
          $fr_check = isset($list[$row->french_vid]);
          if ($en_check xor $fr_check) {
            unset($list[$row->english_vid]);
            unset($list[$row->french_vid]);
            drupal_set_message(
                t('A revision was not deleted because the linked node was not
                  also selected for deletion'), 'warning');
          }
        }
      }
  }
  return $list;
}

/**
 * Delete revisions properly
 *
 * @param array $list
 *   List of objects with nid/vid populated
 */
function uottawa_lang_editor_old_revisions_delete($list) {
  // always sanitize them before deleting them
  $list = uottawa_lang_editor_old_revisions_sanitize_deletions($list);
  foreach ($list as $item) {
    node_revision_delete($item->vid);
  }
  // @todo shouldn't we be deleting the links from our table as well?
}

/**
 * Builds a list of old revisions that are ready to be deleted.
 *
 * Criteria are:
 * - not the current revision
 * - if a major revision, created at least X seconds ago
 * - if a minor revision, created at least Y seconds ago
 * -
 *
 * @param boolean $cron
 *   Is this being populated for cron deletion or not
 *
 * @return array
 *   An array of stdClass objects to be deleted (if cron is true), otherwise a
 *   render array of each item as generated by
 *   uottawa_lang_editor_get_revision_line().
 */
function uottawa_lang_editor_get_old_revisions($cron = FALSE) {
  $entries = array();
  $query = db_select('node_revision', 'nr');
  $query->addField('nr', 'nid');
  $query->addField('nr', 'vid');
  if (!$cron) {
    $query->addField('nr', 'uid');
    $query->addField('nr', 'title');
    $query->addField('nr', 'log');
    $query->addField('nr', 'timestamp');
  }
  $conds = db_or();
  $cond1 = db_and();
  $cond1->condition(db_or()
          ->condition('log', 'LAN:: %', 'LIKE')
          ->condition('log', 'LAM:: %', 'LIKE')
  );
  // @todo shouldn't we be ignoring this condition under some circumstances
  $cond1->condition('timestamp', time() -
      variable_get('uottawa_lang_editor_decay_minor'), '<=');
  $cond2 = db_and();
  $cond2->condition('log', 'LAN:: %', 'NOT LIKE');
  $cond2->condition('log', 'LAM:: %', 'NOT LIKE');
  // @todo same note as above
  $cond2->condition('timestamp', time() -
      variable_get('uottawa_lang_editor_decay_major'), '<=');
  $conds->condition($cond1)->condition($cond2);
  $query->condition($conds);
  $query->orderBy('timestamp', 'DESC');
  $rs = $query->execute();
  $ids = array();
  foreach ($rs as $row) {
    $node = node_load($row->nid);
    if ($node->vid != $row->vid) {
      if (!$cron) {
        $entries[$row->nid . "-" . $row->vid] =
            uottawa_lang_editor_get_revision_line($row);
      }
      else {
        $class = new stdClass();
        $class->nid = $row->nid;
        $class->vid = $row->vid;
        $entries[$row->vid] = $class;
      }
    }
  }
  return $entries;
}


/**
 * Admin form for editing bundle settings.
 */
function uottawa_lang_editor_bundle_form() {
  $form = array(
    "markup" => array(
      "#markup" => "<h1>" . t("Content Types") . "</h1><p>" .
          t("Check the checkbox in order to enable dual-language translation
            for that entity") . '</p>'
    ),
    "entities" => array(
      "#title" => t("Content Types"),
      "#type" => "checkboxes",
      "#options" => array(),
      "#default_value" => array()
    ),
    "submit" => array(
      "#type" => "submit",
      "#value" => t("Save Settings")
    )
  );
  module_load_include('inc', 'uottawa_lang_editor',
      'uottawa_lang_editor.functions');
  $types = node_type_get_types();
  // loop through all the types
  foreach ($types as $machine => $type) {
    // add an option for the type
    $form["entities"]["#options"][$machine] = $type->name;
    // check to see if the value should be checked or not
    if (uottawa_lang_editor_is_dual_entry($machine)) {
      $form["entities"]["#default_value"][] = $machine;
    }
  }
  return $form;
}

/**
 * Processes bundle settings.
 */
function uottawa_lang_editor_bundle_form_submit($form, $form_state) {
  foreach ($form_state["values"]["entities"] as $mname => $type) {
    if (!empty($type)) {
      // $mname has been checked, let's set it to be language-dependent
      db_merge("uottawa_lang_editor_bundle_settings")
          ->key(array("bundle_type" => $mname))
          ->fields(array(
            "bundle_type" => $mname,
            "language_option" => "D"
          ))
          ->execute();
    }
    else {
      // $mname has not been checked, let's set it to be language-independent
      db_merge("uottawa_lang_editor_bundle_settings")
          ->key(array("bundle_type" => $mname))
          ->fields(array(
            "bundle_type" => $mname,
            "language_option" => "I"
          ))
          ->execute();
    }
  }
  drupal_set_message(t("The entity settings have been saved"));
}

/**
 * The fields settings form.
 */
function uottawa_lang_editor_field_form() {
  // extra CSS for the editor
  drupal_add_css(drupal_get_path("module", "uottawa_lang_editor") .
      "/uottawa_lang_editor_admin.css");
  $form = array(
    "markup" => array(
      "#markup" => "<h1>" . t("Field Types") . "</h1></p>"
    ),
    "entities" => array(
      "#title" => "",
      "#type" => "container",
      "#tree" => TRUE
    ),
    "submit" => array(
      "#type" => "submit",
      "#value" => t("Save Settings")
    )
  );

  $types = node_type_get_types();
  $field_list = array();
  $settings = array();
  // loop through all the types
  module_load_include('inc', 'uottawa_lang_editor',
      'uottawa_lang_editor.functions');
  foreach ($types as $machine => $type) {
    // we only want to process dual-entry content types

    if (uottawa_lang_editor_is_dual_entry($machine)) {
      // add a form entry for it
      $form["entities"][$machine] = array(
        "#title" => check_plain($type->name),
        "#type" => "fieldset",
        "#collapsible" => TRUE,
        "#collapsed" => TRUE,
        "#tree" => TRUE,
      );
      // holds all the current settings
      $settings[$machine] = array();
      // holds all the fields
      $field_list[$machine] = array();
      $info = field_info_instances("node", $machine);
      // for each field
      foreach ($info as $mach => $piece) {
        // add the field to the list
        $field_list[$machine][$mach] = $piece["label"];
        // find the default setting for it
        $settings[$machine][$mach] = variable_get("uottawa_lang_editor_field_default", "D");
      }
    }
  }
  // look for all the fields that have already been saved
  $rs = db_query("SELECT * FROM {uottawa_lang_editor_field_settings}");
  foreach ($rs as $row) {
    // if it's a dual-entry content type
    if (isset($field_list[$row->bundle_type])) {
      // if the field has already been set, update it
      if (!isset($field_list[$row->bundle_type][$row->field_name])) {
        $field_list[$row->bundle_type][$row->field_name] = $row->field_name;
      }
      // otherwise add it
      $settings[$row->bundle_type][$row->field_name] = $row->language_option;
    }
  }
  foreach ($field_list as $bundle_machine => $fields) {
    foreach ($fields as $field_machine => $title) {
      // add an entry for this field to the FAPI array
      $form["entities"][$bundle_machine][$field_machine] = array(
        "#type" => "select",
        "#attributes" => array(
          "class" => array("uottawa_lang_editor_side_by_side")
        ),
        // translate the title if needed
        "#title" => check_plain(uottawa_lang_editor_field_title($title)),
        "#options" => array(
          "D" => t("Language Dependent"),
          "I" => t("Language Independent"),
          "H" => t("Hidden"),
        ),
        "#default_value" => $settings[$bundle_machine][$field_machine]
      );
    }
  }
  return $form;
}

/**
 * Returns a more human-friendly title for a given fieldname.
 *
 * @param string $title
 *   The original title
 *
 * @return string
 *   The new title
 */
function uottawa_lang_editor_field_title($title) {
  switch ($title) {
    case "title": return "Title";
    case "revision_information": return "Revisions";
    case "author": return "Author";
    case "options": return "Published Settings";
    case "comment_settings": return "Comment Settings";
    case "menu": return "Menu Settings";
    case "path": return "Alias Settings";
  }
  return t($title);
}

/**
 * Handles the submit event for updating fields.
 */
function uottawa_lang_editor_field_form_submit($form, $state) {
  foreach ($state["values"]["entities"] as $bundle_type => $fields) {
    foreach ($fields as $field_name => $language_option) {
      // save/merge the settings into the database
      db_merge("uottawa_lang_editor_field_settings")
          ->key(array(
            "field_name" => $field_name,
            "bundle_type" => $bundle_type
          ))
          ->fields(array(
            "field_name" => $field_name,
            "bundle_type" => $bundle_type,
            "language_option" => $language_option
          ))
          ->execute();
    }
  }
  drupal_set_message(t("Field settings saved successfully!"));
}