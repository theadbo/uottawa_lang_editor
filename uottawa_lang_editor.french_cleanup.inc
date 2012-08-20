<?php
/**
 * @file
 * Holds the UOttawaLanguageEditor_FrenchCleanup class, which is responsible
 * for cleaning up references in the French side of a dual-language form.
 */


  /**
   * Provides a static interface for cleaning up various properties of
   * the French version of form elements.
   */
  class UOttawaLanguageEditor_FrenchCleanup {

    /**
     * Stores tokens to check javascript against
     *
     * Note that JS token searching is done using
     * strpos($value, $token) !== false
     *
     * Format is $token => $replacement
     *
     * @var array
     */
    private $js_tokens = array();

    /**
     * Stores tokens to check classes against
     *
     * Class checking is absolute ($token == $value)
     *
     * Format is $token => $replacement
     *
     * @var array
     */
    private $class_tokens = array();

    /**
     * Stores tokens to check states against
     *
     * State checking is absolute ($token == $value)
     *
     * Format is $token => $replacement
     *
     * @var array
     */
    private $state_tokens = array();

    /**
     * Stores tokens to check validation methods against
     *
     * Validation method checking is absolute ($token == $value)
     *
     * Format is $token => $replacement
     *
     * @var array
     */
    private $validation_tokens = array();

    /**
     * Static instance of this class (singleton pattern)
     * @var UOttawaLanguageEditor_FrenchCleanup
     */
    private static $instance = NULL;

    /**
     * Obtains the static instance of this class (singleton pattern)
     * @return UOttawaLanguageEditor_FrenchCleanup
     */
    public static function i() {
      if (self::$instance == NULL) {
        self::$instance = new UOttawaLanguageEditor_FrenchCleanup();
      }
      return self::$instance;
    }

    /**
     * Private constructor (singleton pattern)
     */
    private function __construct() {
      $this->init_js_tokens();
      $this->init_class_tokens();
      $this->init_state_tokens();
      $this->init_validation_tokens();
    }

    /**
     * Initializes the tokens to check validation input against
     *
     * You should add additional tokens to match validation elements here, in
     * the format:
     * validation_function_name => replacement_validation_function_name
     */
    private function init_validation_tokens() {
      // this validation method causes problems, so we've replaced it completely
      $this->validation_tokens["path_form_element_validate"] = "uottawa_lang_editor_french_path_form_element_validate";
    }

    /**
     * Initializes the tokens to check javascript files against
     *
     * You should add additional tokens to match validation elements here, in
     * the format:
     * identifying_js_file_fragment => path_to_replacement_js_file
     */
    private function init_js_tokens() {
      $lang_editor_path = drupal_get_path("module", "uottawa_lang_editor");
      $this->js_tokens["/node/node.js"] = $lang_editor_path . "/node.js";
      $this->js_tokens["/path/path.js"] = $lang_editor_path . "/path.js";
      $this->js_tokens["/pathauto/pathauto.js"] = $lang_editor_path . "/pathauto.js";
      $this->js_tokens["/menu/menu.js"] = $lang_editor_path . "/menu.js";
      $this->js_tokens["/metatag/metatag.vertical-tabs.js"] = $lang_editor_path . "/metatag.vertical-tabs.js";
    }

    /**
     * Initializes the tokens to check classes against
     *
     * You should add additional tokens to match classes here, in the format:
     * class_name => replacement_class_name
     */
    private function init_class_tokens() {
      $this->class_tokens["node-form-revision-information"] = "node-form-revision-information-french";
      $this->class_tokens["node-form-author"] = "node-form-author-french";
      $this->class_tokens["node-form-options"] = "node-form-options-french";
      $this->class_tokens["path-form"] = "path-form-french";
      $this->class_tokens["menu-link-form"] = "menu-link-form-french";
      $this->class_tokens["metatags-form"] = "metatags-form-french";
    }

    /**
     * Initializes the tokens to check state values against
     *
     * You should add additional tokens to match state values here, in the
     * format:
     * original_jquery => replacement_query
     */
    private function init_state_tokens() {
      $this->state_tokens['textarea[name="log"]'] = 'textarea[name="log_french"]';
      $this->state_tokens['input[name="path[pathauto]"]'] = 'input[name="path_french[pathauto_french]"]';
      $this->state_tokens['input[name="menu[enabled]"]'] = 'input[name="menu_french[enabled_french]"]';
    }

    /**
     * Checks a condition based on two operands and the $mode
     * @param mixed $left The first operand
     * @param mixed $right The second operand
     * @param string $mode The identifying mode to compare $left and $right with
     * @return boolean True if they match, otherwise false
     */
    private function check_condition($left, $right, $mode = "equals") {
      switch ($mode) {
        case "contains":
          return strpos($left, $right) !== FALSE;
        default:
        case "equals":
          return $left == $right;
      }
    }

    /**
     * Cleans up $data by replacing the tokens in $tokenArray with their
     *   replacements
     *
     * @param array $tokenArray An associative array of replacement values
     * @param array $data The data to find replacements for
     * @param string $mode The comparison mode to be used
     * @param boolean $targetValues If true, checks for the tokens in the array
     *   values of $data. Otherwise, checks for the tokens in the keys of $data.
     * @return array The cleaned up version of $data
     */
    private function cleanup_tokens(&$tokenArray, $data, $mode = "equals", $targetValues = TRUE) {
      if (!is_array($data)) {
        $data = array($data);
      }
      $new_data = array();
      foreach ($data as $key => $val) {
        $found = FALSE;
        foreach ($tokenArray as $token => $replacement) {
          // we only want to replace things once
          if (!$found) {
            // if we're looking for values
            if ($targetValues) {
              // we don't want to process arrays of values
              if (!is_array($val)) {
                // check the condition
                if ($this->check_condition($val, $token, $mode)) {
                  // do the replacement
                  $found = TRUE;
                  $new_data[$key] = $replacement;
                }
              }
            // if we're looking for keys
            }
            else {
              // check the condition
              if ($this->check_condition($key, $token, $mode)) {
                // do the replacement
                $found = TRUE;
                $new_data[$replacement] = $val;
              }
            }
          }
        }
        // if we didn't do a replacement, keep the original
        if (!$found) {
          $new_data[$key] = $val;
        }
      }
      // return
      return $new_data;
    }

    /**
     * Cleans up validation information for a french element
     * @param array $data The array of validation elements
     * @return array Cleaned up version
     */
    public function cleanup_validation_tokens($data) {
      return $this->cleanup_tokens($this->validation_tokens, $data, "equals");
    }

    /**
     * Cleans up state information for a french element
     * @param array $data The array of state elements (note this is the list of
     *   actual jQuery => condition elements)
     * @return array Cleaned up version
     */
    public function cleanup_state_tokens($data) {
      return $this->cleanup_tokens($this->state_tokens, $data, "equals", FALSE);
    }

    /**
     * Cleans up js file information for a french element
     * @param array $data The array of js files
     * @return array Cleaned up version
     */
    public function cleanup_js_tokens($data) {
      return $this->cleanup_tokens($this->js_tokens, $data, "contains");
    }

    /**
     * Cleans up class information for a french element
     * @param array $data The array of classes
     * @return array Cleaned up version
     */
    public function cleanup_class_tokens($data) {
      return $this->cleanup_tokens($this->class_tokens, $data, "equals");
    }

  }